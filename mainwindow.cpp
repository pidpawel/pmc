/*
 * Program: Symulator Przykładowej Maszyny Cyfrowej
 * Autor: Paweł Kozubal
 * Licencja: Użytek inny niż edukacyjny wymaga zgody autora.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));

    ui->setupUi(this);

    ui->runtimeSplitter->hide();
    ui->runtimeSplitter->setStretchFactor(0,3);
    ui->runtimeSplitter->setStretchFactor(1,1);

    this->mach = new pmc();
    this->setVerbosity(9);

    connect(ui->programTable, SIGNAL(cellClicked(int,int)), this, SLOT(programCellClicked(int, int)));
    connect(ui->programTable, SIGNAL(cellChanged(int,int)), this, SLOT(programCellChanged(int, int)));
    connect(ui->startPCEdit, SIGNAL(textChanged(QString)), this, SLOT(pmcSetStartPC(QString)));

    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));

    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(newAction()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openAction()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveAction()));
    connect(ui->actionSaveAs, SIGNAL(triggered()), this, SLOT(saveAsAction()));
    connect(ui->programErase, SIGNAL(clicked()), this, SLOT(clearProgram()));
    connect(ui->programDelete, SIGNAL(clicked()), this, SLOT(deleteLastProgramRow()));
    connect(ui->programAdd, SIGNAL(clicked()), this, SLOT(appendAndEditProgramRow()));

    connect(this->mach, SIGNAL(log(int, QString)), this, SLOT(log(int, QString)));
    connect(this->mach, SIGNAL(stopped()), this, SLOT(pmcStop()));

    connect(this->mach, SIGNAL(memoryResized(int16_t)), this, SLOT(pmcMemoryResized(int16_t)));
    connect(this->mach, SIGNAL(memoryChanged(int16_t,int16_t)), this, SLOT(pmcMemoryChanged(int16_t,int16_t)));
    connect(this->mach, SIGNAL(memoryRead(int16_t)), this, SLOT(pmcMemoryHilight(int16_t)));

    connect(this->mach, SIGNAL(PCChanged(int16_t)), this, SLOT(pmcPCChanged(int16_t)));
    connect(this->mach, SIGNAL(ACChanged(int16_t)), this, SLOT(pmcACChanged(int16_t)));
    connect(this->mach, SIGNAL(ORChanged(int16_t)), this, SLOT(pmcORChanged(int16_t)));

    connect(this->mach, SIGNAL(PCRead()), this, SLOT(pmcPCHilight()));
    connect(this->mach, SIGNAL(ORRead()), this, SLOT(pmcORHilight()));
    connect(this->mach, SIGNAL(ACRead()), this, SLOT(pmcACHilight()));

    connect(ui->actionStart, SIGNAL(changed()), this, SLOT(pmcStartStop()));
    connect(ui->controlReset, SIGNAL(clicked()), this, SLOT(pmcReset()));
    connect(ui->controlSkip, SIGNAL(clicked()), this, SLOT(pmcStep()));
    connect(ui->controlRun, SIGNAL(toggled(bool)), this, SLOT(pmcRunToggled(bool)));

    ui->controlRun->setCheckable(true);
    connect(ui->verbositySlider, SIGNAL(sliderMoved(int)), this, SLOT(setVerbosity(int)));
    connect(ui->runSpeedSlider, SIGNAL(valueChanged(int)), this, SLOT(pmcSetStepDelay(int)));

    this->stepTimer = new QTimer();
    this->pmcSetStepDelay(500);
    connect(this->stepTimer, SIGNAL(timeout()), this, SLOT(stepTimerEvent()));

    this->newAction();
    this->inImportMode = false;

    appendAndEditProgramRow();
}

void MainWindow::pmcReset(){
    ui->actionStart->setChecked(false);
    ui->actionStart->setChecked(true);
    this->pmcResetHilights();
}

void MainWindow::pmcStop(){
    ui->controlRun->setChecked(false);
    ui->controlRun->setDisabled(true);
    ui->controlSkip->setDisabled(true);
}

void MainWindow::pmcStep(){
    this->pmcResetHilights();
    this->mach->step();
    if(this->verbosity>3)
        ui->logEdit->append("--------------------------------------------------");
}

void MainWindow::pmcRunToggled(bool state){
    if(state){
        this->stepTimer->start();
    }else{
        this->stepTimer->stop();
    }
}

void MainWindow::stepTimerEvent(){
    if(ui->controlRun->isChecked() && this->mach->isRunning()){
        this->pmcStep();
    }
    if(!this->mach->isRunning())
        this->stepTimer->stop();
}

void MainWindow::pmcStartStop(){
    if(ui->actionStart->isChecked()){
        ui->logEdit->clear();
        this->log(5,tr("Przygotowywanie maszyny…"));
        ui->programTable->setDisabled(true);
        ui->programAdd->setDisabled(true);
        ui->programDelete->setDisabled(true);
        ui->programErase->setDisabled(true);

        ui->controlRun->setDisabled(false);
        ui->controlSkip->setDisabled(false);

        ui->runtimeTable->clearContents();
        ui->runtimeTable->setRowCount(0);

        int count = ui->programTable->rowCount();
        this->mach->reset();
        this->mach->setStartPC((int16_t)ui->startPCEdit->text().toInt());
        this->mach->setMemorySize(count);

        bool ok;
        QTableWidgetItem *item;
        for(int i=0; i<count; i++){
            item = ui->programTable->item(i, 1);
            if(item != NULL){
                this->mach->setCell(i, item->text().toInt(&ok, 2));
            }else
                this->log(0, "BŁĄD PROGRAMU");
        }

        ui->programWidget->hide();
        ui->programTable->setDisabled(false);
        ui->runtimeSplitter->show();
        ui->controlSkip->setDisabled(false);
        ui->logEdit->append("--------------------------------------------------");
    }else{
        ui->programAdd->setDisabled(false);
        ui->programDelete->setDisabled(false);
        ui->programErase->setDisabled(false);
        ui->controlSkip->setDisabled(true);
        ui->runtimeSplitter->hide();
        ui->programWidget->show();
        ui->logEdit->append(tr("Zatrzymano."));
    }
}

void MainWindow::pmcMemoryResized(int16_t newSize){
    int old = ui->runtimeTable->rowCount();
    ui->runtimeTable->setRowCount(newSize);
    for(int i=old; i<newSize; i++){
        QTableWidgetItem *rowLP = new QTableWidgetItem(QString::number(i));
        rowLP->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);
        rowLP->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->runtimeTable->setItem(i, 0, rowLP);
    }
}

void MainWindow::pmcMemoryChanged(int16_t address, int16_t newVal){
    QTableWidgetItem *bin = new QTableWidgetItem( QString("%1").arg((int)(newVal & 0xFFFF), 16, 2, QChar('0')) );
    bin->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);
    ui->runtimeTable->setItem(address, 1, bin);

    QTableWidgetItem *command = new QTableWidgetItem(this->mach->registryToCommand(newVal));
    command->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);
    ui->runtimeTable->setItem(address, 2, command);

    QTableWidgetItem *dec = new QTableWidgetItem(QString::number(newVal));
    dec->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);
    ui->runtimeTable->setItem(address, 3, dec);

    pmcMemoryHilight(address);
    ui->runtimeTable->resizeColumnsToContents();
}

void MainWindow::pmcPCChanged(int16_t newValue){
    ui->PCDecLCD->display(newValue);
    pmcPCHilight();
}
void MainWindow::pmcACChanged(int16_t newValue){
    ui->ACBinLCD->display((unsigned int16_t)newValue);
    ui->ACDecLCD->display(newValue);
    pmcACHilight();
}
void MainWindow::pmcORChanged(int16_t newValue){
    ui->ORBinLCD->display((unsigned int16_t)newValue);
    ui->ORDecLCD->display(newValue);
    pmcORHilight();
}

void MainWindow::pmcResetHilights(){
    QFont font = ui->PCLabel->font();
    font.setBold(false);
    ui->PCLabel->setFont(font);
    font = ui->ORLabel->font();
    font.setBold(false);
    ui->ORLabel->setFont(font);
    font = ui->ACLabel->font();
    font.setBold(false);
    ui->ACLabel->setFont(font);

    for(int16_t j=0; j<ui->runtimeTable->rowCount(); j++){
        for(int i=0; i<4; i++){
            QTableWidgetItem *current = ui->runtimeTable->item(j, i);
            QFont font = current->font();
            font.setWeight(QFont::Normal);
            current->setFont(font);
        }
    }
}
void MainWindow::pmcPCHilight(){
    QFont font = ui->PCLabel->font();
    font.setBold(true);
    ui->PCLabel->setFont(font);
}
void MainWindow::pmcORHilight(){
    QFont font = ui->ORLabel->font();
    font.setBold(true);
    ui->ORLabel->setFont(font);
}
void MainWindow::pmcACHilight(){
    QFont font = ui->ACLabel->font();
    font.setBold(true);
    ui->ACLabel->setFont(font);
}
void MainWindow::pmcMemoryHilight(int16_t addr){
    for(int i=0; i<4; i++){
        QTableWidgetItem *current = ui->runtimeTable->item((int)addr, i);
        QFont font = current->font();
        font.setWeight(QFont::Bold);
        current->setFont(font);
    }
}

void MainWindow::pmcSetStartPC(QString text){
    this->mach->setStartPC((int16_t)text.toInt());
}
void MainWindow::pmcSetStepDelay(int delay){
    this->stepTimer->setInterval(delay);
    if(QObject::sender() != ui->runSpeedSlider)
        ui->runSpeedSlider->setValue(delay);
}

int MainWindow::appendNewProgramRow(){
    disconnect(ui->programTable, SIGNAL(cellChanged(int,int)), this, SLOT(programCellChanged(int, int)));
    int row_no = ui->programTable->rowCount();
    ui->programTable->setRowCount(row_no+1);

    QTableWidgetItem *rowLP = new QTableWidgetItem(QString::number(row_no));
    rowLP->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);
    rowLP->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->programTable->setItem(row_no, 0, rowLP);

    QTableWidgetItem *rowInt = new QTableWidgetItem(QString("0000000000000000"));
    rowInt->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);
    ui->programTable->setItem(row_no, 1, rowInt);

    QTableWidgetItem *rowCommand = new QTableWidgetItem(QString("0"));
    rowCommand->setFlags(Qt::NoItemFlags | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    ui->programTable->setItem(row_no, 2, rowCommand);

    ui->programTable->resizeColumnToContents(0);
    ui->programTable->resizeColumnToContents(1);
    connect(ui->programTable, SIGNAL(cellChanged(int,int)), this, SLOT(programCellChanged(int, int)));

    return row_no;
}

void MainWindow::appendAndEditProgramRow(){
    int no = appendNewProgramRow();
    this->rowToEdit = no;
    QTimer::singleShot(20, this, SLOT(editProgramRow()));
}
void MainWindow::editProgramRow(){
    ui->programTable->editItem(ui->programTable->item(this->rowToEdit, 2));
}

void MainWindow::programCellClicked(int row, int col){
    if(col==2)
        ui->programTable->editItem(ui->programTable->item(row, col));
}

void MainWindow::programCellChanged(int row, int col){
    if(col == 2){
        QString cell = ui->programTable->item(row, col)->text().toUpper();

        int16_t registry;
        int newVal = this->mach->parseString(cell, &registry);

        QTableWidgetItem *newValB = new QTableWidgetItem();
        newValB->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);

        if(newVal < 0){
            this->showErrorPopup(
                                tr("Błąd w wyrażeniu"),
                                tr("Błąd wystąpił w interpretacji polecenia z adresu %1").arg(row));
            newValB->setText(tr("Błąd"));
            ui->programTable->setItem(row, 1, newValB);
            ui->programTable->resizeColumnToContents(1);
        }else{
            newValB->setText( QString("%1").arg((int)(registry & 0xFFFF), 16, 2, QChar('0')) );
            ui->programTable->setItem(row, 1, newValB);
            ui->programTable->resizeColumnToContents(1);
            if(!this->inImportMode && row == (ui->programTable->rowCount()-1)){
                this->appendAndEditProgramRow();
            }
        }
    }
}

void MainWindow::deleteLastProgramRow(){
    ui->programTable->removeRow(ui->programTable->rowCount()-1);
}

void MainWindow::clearProgram(){
    ui->programTable->clearContents();
    ui->programTable->setRowCount(0);
    if(!this->inImportMode)
        this->appendAndEditProgramRow();
}

void MainWindow::setVerbosity(int newVerbosity){
    this->verbosity = newVerbosity;
    if(QObject::sender() != ui->verbositySlider)
        ui->verbositySlider->setValue(newVerbosity);
}

void MainWindow::log(int level, QString text){
    ui->statusBar->showMessage(text, 0);
    if(level <= this->verbosity){
        ui->logEdit->append(text);
    }
    if(level == 0)
        this->close();
}

void MainWindow::showErrorPopup(QString title, QString text){
    QMessageBox popup;
    popup.setIcon(QMessageBox::Critical);
    popup.setText(title);
    popup.setInformativeText(text);
    popup.exec();
}

void MainWindow::newAction(){
    this->filename = QString("");
    this->clearProgram();
    ui->actionStart->setChecked(false);
}
void MainWindow::openAction(){
    this->filename = QFileDialog::getOpenFileName(this, tr("Otwórz plik"),
                                                     "",
                                                     tr("Files (*.*)"));
    QFile file(this->filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        this->showErrorPopup("Błąd importu", "Nie można otworzyć pliku!");
        return;
    }

    this->inImportMode = true;
    this->clearProgram();

    QTextStream in(&file);
    QString line = in.readLine();
    int lineno;

    if(!line.isNull()){
        ui->startPCEdit->setText(line);
        line = in.readLine();
    }
    while (!line.isNull()){
        lineno = this->appendNewProgramRow();
        ui->programTable->item(lineno, 2)->setText(line);
        line = in.readLine();
    }
    this->inImportMode = false;
    this->log(2, tr("Import zakończono pomyślnie!"));
    ui->actionStart->setChecked(false);
}

void MainWindow::saveAsAction(){
    this->filename = QFileDialog::getSaveFileName(this, tr("Zapisz plik jako"),
                                                     "",
                                                     tr("Files (*.*)"));
    if(this->filename.length()>0)
        this->saveAction();
}

void MainWindow::saveAction(){
    if(this->filename.length()==0)
        this->saveAsAction();

    QFile file(this->filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        this->showErrorPopup("Błąd eksportu", "Nie można otworzyć pliku do zapisu! Plik nie istnieje bądź nie masz uprawnień do posługiwania się nim.");
        return;
    }

    QTextStream out(&file);
    out << ui->startPCEdit->text() << "\n";
    for(int i=0; i<ui->programTable->rowCount(); i++){
        out << ui->programTable->item(i, 2)->text() << "\n";
    }
    this->log(2, tr("Zapisano!"));
}

void MainWindow::showAbout(){
    QMessageBox::about(this, tr("Symulator Przykładowej Maszyny Cyfrowej"),
                       tr("Program <i>Symulator Przykładowej Maszyny Cyfrowej</i> powstał jako zadanie na ćwiczenia z przedmiotu <i>„Wstęp do Informatyki”</i>. <br /><br />Autor: Paweł Kozubal<br /><br />Specyfikacja <i>Przykładowej Maszyny Cyfrowej</i> zawarta jest w skrypcie do przedmiotu, którego autorami są <i>dr Jacek Lembas</i> i <i>dr Rafał Kawa.</i>"));
}

MainWindow::~MainWindow()
{
    delete ui;
}
