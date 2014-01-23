#ifndef PMC_H
#define PMC_H

#include <QObject>
#include <QDebug>
#include <QString>
#include <QStringList>

#define BIT_MASK 0x7fff

class pmc : public QObject
{
    Q_OBJECT

public:
    pmc();

    void setMemorySize(int16_t newSize);
    void setCell(int16_t address, int16_t value);

    void setStartPC(int16_t value);

    int16_t getCell(int16_t address);
    int16_t getPC();
    int16_t getOR();
    int16_t getAC();

    bool isRunning();

    int registryToint(int16_t inp);

    const static char *command_list[];
    const static char addr_types[];

    static int16_t parseString(QString text, int16_t *registry);
    static int16_t parseCommand(QString command, char addrType, QString arg);
    static int validateCommand(QString command);
    static QString registryToCommand(int16_t registry);

signals:
    void log(int level, QString text);

    void PCChanged(int16_t newValue);
    void ORChanged(int16_t newValue);
    void ACChanged(int16_t newValue);

    void PCRead();
    void ORRead();
    void ACRead();

    void memoryResized(int16_t newSize);
    void memoryChanged(int16_t addr, int16_t newVal);
    void memoryRead(int16_t addr);

    void started();
    void stopped();

public slots:
    void reset();
    int step();

private:
    int16_t *memory, memory_size;
    int16_t PC, OP, AC, startPC;
    bool running;

    void setPC(int16_t newValue);
    void setOR(int16_t newValue);
    void setAC(int16_t newValue);

    void setRunning(bool w);
};

#endif // PMC_H
