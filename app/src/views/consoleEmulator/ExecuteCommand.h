#ifndef _EXECUTECOMMAND_H_
#define	_EXECUTECOMMAND_H_

#include <QWidget>
#include <QDialog>
#include <QProcess>


class ConsoleEmulator;


class ExecuteCommand : public QObject
{
 Q_OBJECT

public:
 ExecuteCommand(QObject *parent=nullptr);
 virtual ~ExecuteCommand();

 static QString getOsFamily();

 void setCommand(QString cmd);
 void run(bool visible=true);
 int runSimple();

 void setWindowTitle(QString title);
 void setMessageText(QString text);

signals:
 void finishWork();

private slots:

 void manualCloseProcess(void);
 void errorHanler(QProcess::ProcessError error);

private:

 QString command;
 QString shell;
 QString windowTitle;
 QString messageText;

 QProcess *process;
 QTextCodec *outputCodec;

 ConsoleEmulator *console;

 bool isError;
 bool isManualClose;

 void printOutput(QProcess *process, ConsoleEmulator *console);
 void closeProcess(void);
};

#endif	/* _EXECUTECOMMAND_H_ */

