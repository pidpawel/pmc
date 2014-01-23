#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextCodec>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QTimer>
#include "pmc.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private slots:
    void programCellClicked(int, int);
    void programCellChanged(int, int);
    int appendNewProgramRow(); // Dla importu zrobić przeciążoną funkcję
    void appendAndEditProgramRow();
    void editProgramRow();
    void deleteLastProgramRow();
    void clearProgram();

    void stepTimerEvent();

    void pmcMemoryResized(int16_t newSize);
    void pmcMemoryChanged(int16_t address, int16_t newVal);

    void pmcStartStop();
    void pmcReset();
    void pmcStop();
    void pmcStep();
    void pmcRunToggled(bool state);

    void pmcPCChanged(int16_t newValue);
    void pmcACChanged(int16_t newValue);
    void pmcORChanged(int16_t newValue);

    void pmcResetHilights();
    void pmcPCHilight();
    void pmcACHilight();
    void pmcORHilight();
    void pmcMemoryHilight(int16_t addr);

    void pmcSetStartPC(QString text);

    void pmcSetStepDelay(int);

    void setVerbosity(int newVerbosity);
    void log(int level, QString text);
    void showErrorPopup(QString title, QString text);

    void saveAction();
    void saveAsAction();
    void openAction();
    void newAction();

    void showAbout();

private:
    Ui::MainWindow *ui;
    pmc *mach;
    int verbosity;
    int rowToEdit; // Dirty hack! Ale pomaga bo nie można z akcji od edytowania pola przejść do edycji innego pola.
    bool inImportMode; // Kolejny dirty hack :)
    QTimer *stepTimer;
    QString filename;
};

#endif // MAINWINDOW_H
