#include "pmc.h"

const char *pmc::command_list[] = {"STOP", "LOAD", "STORE", "JUMP", "JNEG", "JZERO", "ADD", "SUB", "MULT", "DIV", "AND", "OR", "NOT", "CMP", "SHZ", "SHC"};
const char pmc::addr_types[] = {'$', '@', '&', '+'};

pmc::pmc()
{
    this->startPC = 0;
    this->memory = (int16_t*)malloc(sizeof(int16_t));
    this->reset();
}
void pmc::reset(){
    emit log(6, QString("Wyzerowano rejestry."));
    this->setPC(this->startPC);
    this->setAC(0);
    this->setOR(0);
    this->running = true;
}

int pmc::step(){
    int16_t cur_pc = this->getPC();

    if(cur_pc >= this->memory_size){
        emit log(1, tr("Próba wykonania kodu z nieistniejącego adresu w pamięci!"));
        this->setRunning(false);
        return -1;
    }

    int16_t registry = this->getCell(cur_pc);
    int16_t command =  (registry >> 11) & 0xf;
    int16_t addrType = (registry >> 9 ) & 0x3;
    int16_t arg =       registry        & 0x1ff;

    int16_t tmpint16, tmpint2;
    bool tmpbool;

    this->setPC(cur_pc+1);

    if(command != 0){ // STOP nie wymaga operanda
        switch(addrType){
            case 0: this->setOR( arg ); break;
            case 1: this->setOR( this->getCell(arg) ); break;
            case 2: this->setOR( this->getCell(this->getCell(arg)) ); break;
            case 3: this->setOR( this->getCell(arg+this->getAC()) ); break;
        }
    }

    emit log(2, tr("Wykonywanie komendy:") + this->command_list[command]);

    switch(command){
    case 0: // STOP
        emit log(1, tr("Zatrzymano instrukcją STOP."));
        this->setRunning(false);
        return 1; // Kod: zatrzymano instrukcją STOP
        break;
    case 1: // LOAD
        this->setAC(this->getOR());
        break;
    case 2: // STORE
        this->setCell(this->getOR(), this->getAC());
        break;
    case 3: // JUMP
        this->setPC(this->getOR());
        break;
    case 4: // JNEG
        if(this->getAC() < 0)
            this->setPC(this->getOR());
        break;
    case 5: // JZERO
        if(this->getAC() == 0)
            this->setPC(this->getOR());
        break;
    case 6: // ADD
        this->setAC(this->getAC() + this->getOR());
        break;
    case 7: // SUB
        this->setAC(this->getAC() - this->getOR());
        break;
    case 8: // MULT
        this->setAC(this->getAC() * this->getOR());
        break;
    case 9: // DIV
        tmpint2 = this->getOR();
        if(tmpint2==0){
            emit log(5, tr("Dzielenie przez zero!"));
            this->setAC(0);
        }else{
            this->setAC(this->getAC() / tmpint2);
        }
        break;
    case 10: // AND
        this->setAC(this->getAC() & this->getOR());
        break;
    case 11: // OR
        this->setAC(this->getAC() | this->getOR());
        break;
    case 12: // NOT
        this->setAC(~this->getOR());
        break;
    case 13: // CMP
        if(this->getAC()==this->getOR())
            this->setAC(-1);
        else
            this->setAC(0);
        break;
    case 14: // SHZ
        if(this->getOR()<0)
            this->setAC(this->getAC() >> abs(this->getOR()));
        if(this->getOR()>0)
            this->setAC(this->getAC() << abs(this->getOR()));
        break;
    case 15: // SHC
        tmpint16 = this->getAC();
        tmpint2 = this->getOR();
        for(int i=0; i<(abs(tmpint2)%16); i++){
            if(tmpint2<0){
                tmpbool = (bool)(0x0001 & tmpint16);
                tmpint16 >>= 1;
                tmpint16 |= (tmpbool?0x01:0x00 << 15);
            }else{
                tmpbool = (bool)(0x8000 & tmpint16);
                tmpint16 <<= 1;
                tmpint16 |= tmpbool;
            }
        }
        this->setAC(tmpint16);
        break;
    default:
        emit log(2, tr("Nierozpoznana instrukcja!"));
        return -1;
    }

    return 0;
}

void pmc::setStartPC(int16_t value){
    this->startPC = value;
}

void pmc::setMemorySize(int16_t newSize){
    // Ograniczenie do 512 komórek pamięci
    if(newSize>512) newSize = 512;
    this->memory = (int16_t*)realloc(this->memory, sizeof(int16_t)*newSize);
    this->memory_size = newSize;
    emit memoryResized(newSize);
}

void pmc::setCell(int16_t address, int16_t value){
    if(address >= this->memory_size){
        emit log(1, tr("Próba zapisu do nieistniejącego adresu w pamięci!"));
        this->setRunning(false);
        return;
    }
    this->memory[address] = value;
    emit log(7, tr("Nowa wartość komórki o adresie: ") + QString::number(address));
    emit memoryChanged(address, value);
}
int16_t pmc::getCell(int16_t address){
    if(address >= this->memory_size){
        emit log(1, tr("Próba odczytu z nieistniejącego adresu w pamięci!"));
        this->setRunning(false);
        return -1;
    }
    emit log(7, tr("Odczytano zawartość komórki o adresie: ") + QString::number(address));
    emit memoryRead(address);
    return this->memory[address];
}

void pmc::setPC(int16_t newValue){
    newValue &= 0xFFFF;
    this->PC = newValue;
    emit log(7, tr("Nowa wartość PC: ") + QString::number(newValue));
    emit PCChanged(newValue);
}
void pmc::setOR(int16_t newValue){
    newValue &= 0xFFFF;
    this->OP = newValue;
    emit log(7, tr("Nowa wartość OR: ") + QString::number(newValue));
    emit ORChanged(newValue);
}
void pmc::setAC(int16_t newValue){
    newValue &= 0xFFFF;
    this->AC = newValue;
    emit log(7, tr("Nowa wartość AC: ") + QString::number(newValue));
    emit ACChanged(newValue);
}

int16_t pmc::getPC(){
    emit log(8, tr("Odczytano zawartość PC"));
    emit PCRead();
    return this->PC;
}
int16_t pmc::getOR(){
    emit log(8, tr("Odczytano zawartość OP"));
    emit ORRead();
    return this->OP;
}
int16_t pmc::getAC(){
    emit log(8, tr("Odczytano zawartość AC"));
    emit ACRead();
    return this->AC;
}

bool pmc::isRunning(){
    return this->running;
}
void pmc::setRunning(bool w){
    this->running = w;
    if(w)
        emit started();
    else
        emit stopped();
}

int pmc::registryToint(int16_t inp){
    return (int)((int16_t)inp);
}

int16_t pmc::parseString(QString text, int16_t *registry){
    QRegExp rx("(\\-?\\d+)|(([A-Z]+)( ?([@$&+]?) ?(\\d+))?)");
    rx.indexIn(text);
    QStringList matched = rx.capturedTexts();
    int error = 0;
    if(pmc::validateCommand(matched[3]) < 0){
        if(matched[1].length()==0)
            error = 1;
    }else{
        if(matched[3]=="STOP"){
            if(matched[5].length()>0 || matched[6].length()>0)
                error = 2;
        }else{
            if(matched[6].length()==0)
                error = 3;
        }
    }

    if(error){
        return -error;
    }

    char addrT=matched[5][0].toAscii();
    if(matched[5].length()==0)
        addrT = '$';

    if(matched[1].length() == 0)
        *registry = parseCommand(matched[3], addrT, matched[6]);
    else
        *registry = (int16_t)atoi(matched[1].toAscii());
    return 0;
}

int pmc::validateCommand(QString command){
    if(command[0]==0)
        return -2;
    for(int i=0; i<16; i++){
        if(pmc::command_list[i] == command){
            return i;
        }
    }
    return -1;
}

int16_t pmc::parseCommand(QString command, char addrType, QString arg){
    int16_t ret=0;
    int found=0;

    for(int i=0; i<16; i++){
        if(pmc::command_list[i] == command){
            ret |= i<<11;
            found = 1;
            break;
        }
    }
    if(found == 0)
        return -1;
    found = 0;

    for(int i=0; i<4; i++){
        if(pmc::addr_types[i] == addrType){
            ret |= i<<9;
            found = 1;
            break;
        }
    }
    if(found == 0)
        return -2;

    ret |= arg.toInt() & 0x1ff;

    return ret;
}

QString pmc::registryToCommand(int16_t registry){
    int command =  (registry >> 11) & 0xf;
    int addrType = (registry >> 9 ) & 0x3;
    int arg =       registry        & 0x1ff;
    QString argString = QString::number(arg);

    QString ret;
    ret.reserve(strlen(pmc::command_list[command]) + 2 + argString.length());
    ret.append(pmc::command_list[command]);
    ret.append(" ");
    ret.append(pmc::addr_types[addrType]);
    ret.append(argString);

    return ret;
}
