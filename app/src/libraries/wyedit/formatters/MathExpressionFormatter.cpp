#include <QTextBlock>
#include <QTextFragment>
#include <QDebug>
#include <QFileDialog>
#include <QImage>
#include <QImageReader>
#include <QTextDocumentFragment>
#include <QMessageBox>
#include <QImage>
#include <QTemporaryFile>

#include "MathExpressionFormatter.h"

#include "../Editor.h"
#include "../EditorConfig.h"
#include "../EditorTextArea.h"
#include "../EditorCursorPositionDetector.h"
#include "../EditorMultiLineInputDialog.h"

#include "main.h"
#include "libraries/DiskHelper.h"
#include "views/consoleEmulator/ExecuteCommand.h"


MathExpressionFormatter::MathExpressionFormatter()
{

}


// Исходный код формулы, которая выделена (если выделена единственная картинка формулы, требуется доработка)
QString MathExpressionFormatter::mathExpressionOnSelect(void)
{
    // Блок, в пределах которого находится курсор
    QTextBlock currentBlock = textArea->textCursor().block();
    QTextBlock::iterator it;
    QTextFragment fragment;

    // Если есть выделение
    if(textArea->textCursor().hasSelection())
    {
        // Перебираются фрагметы блока
        for(it = currentBlock.begin(); !(it.atEnd()); ++it)
        {
            fragment = it.fragment();

            // Если фрагмент содержит изображение
            if(fragment.isValid())
                if(fragment.charFormat().isImageFormat ())
                {
                    int fragmentStart=fragment.position();
                    int fragmentEnd=fragmentStart+fragment.length();
                    int selectionStart=textArea->textCursor().selectionStart();
                    int selectionEnd=textArea->textCursor().selectionEnd();

                    // Если начало и конец фрагмента совпадает с координатами выделения
                    // Проверяется и случай, когда блок выделен в обратную сторону
                    if( (fragmentStart==selectionStart && fragmentEnd==selectionEnd) ||
                            (fragmentStart==selectionEnd && fragmentEnd==selectionStart) )
                    {
                        QTextImageFormat imageFormat=fragment.charFormat().toImageFormat();

                        if(imageFormat.isValid()) {
                            return getMathExpressionByImageName(imageFormat.name());
                        }
                    }
                }
        }
    }

    return QString();
}


// Сождержимое формулы (картинки формулы) на которой находится курсор
QString MathExpressionFormatter::mathExpressionOnCursor(void)
{
    // Проверка срабатывает только если нет выделения
    if(textArea->textCursor().hasSelection()==false)
    {
        QTextImageFormat imageFormat = textArea->textCursor().charFormat().toImageFormat();

        if(imageFormat.isValid()) {
            return getMathExpressionByImageName(imageFormat.name());
        }
    }

    return QString();
}


// Получение исходного кода математического выражения по имени картинки в ресурсах документа
QString MathExpressionFormatter::getMathExpressionByImageName(QString resourceImageName)
{
    QImage image=textArea->document()->resource(QTextDocument::ImageResource, QUrl(resourceImageName)).value<QImage>();

    if( !image.isNull() ) {
        QString text=image.text("Description");

        if(text.startsWith( imageDescriptionPrefix )) {
            return text.right( text.size() - imageDescriptionPrefix.size()).trimmed();
        }
    }

    return QString();
}


// Обработка клавиши добавления/редактирования математического выражения
void MathExpressionFormatter::onMathExpressionClicked(void)
{

    // Если выделена картинка формулы
    if(editor->cursorPositionDetector->isImageSelect() && mathExpressionOnSelect().size()>0) {
        qDebug() << "Math expression on select: " << mathExpressionOnSelect();

        editMathExpression( mathExpressionOnSelect() );
        return;
    }

    // Если курсор стоит на картинке с формулой
    if(editor->cursorPositionDetector->isCursorOnImage() && mathExpressionOnCursor().size()>0) {
        qDebug() << "Math expression on cursor: " << mathExpressionOnSelect();

        editMathExpression( mathExpressionOnCursor() );
        return;
    }

    // Иначе математическое выражение не выделено, и срабатывает режим добавления
    addMathExpression();
}


// Вызов окна настройки свойств изображения
void MathExpressionFormatter::onContextMenuEditMathExpression()
{
    // Если выделена картинка формулы
    // Свойства меняются только если картинка выделена, если курсор стоит на картинке с формулой, то этого недостаточно
    if(editor->cursorPositionDetector->isImageSelect() && mathExpressionOnSelect().size()>0) {
        qDebug() << "Math expression on select: " << mathExpressionOnSelect();

        editMathExpression( mathExpressionOnSelect() );
    }
}


void MathExpressionFormatter::addMathExpression(void)
{
    // Открывается окно запроса математического выражения
    QString mathExpressionText=getMathExpressionFromUser();

    insertMathExpressionToTextArea(mathExpressionText);
}


void MathExpressionFormatter::editMathExpression(QString iMathExpressionText)
{
    qDebug() << "Edit math expression: " << mathExpressionOnSelect();

    // Открывается окно запроса математического выражения
    QString mathExpressionText=getMathExpressionFromUser( iMathExpressionText );

    insertMathExpressionToTextArea(mathExpressionText);
}


// Запрос математического выражения от пользователя
QString MathExpressionFormatter::getMathExpressionFromUser(QString iMathExpressionText)
{
    EditorMultiLineInputDialog dialog;

    dialog.setText(iMathExpressionText);
    dialog.setWindowTitle(tr("Edit TeX math expression"));

    // Если не нажато OK
    if(dialog.exec()!=QDialog::Accepted)
        return QString();

    // Если ничего небыло изменено в TeX коде
    if(!dialog.isModified())
        return QString();

    return dialog.getText();
}


void MathExpressionFormatter::createGifFromMathExpression(QString iMathExpression, QString iFileName)
{
    QString mimetexBinaryName="mimetex";
    QString chDirCommand;
    QString mimetexPath=QCoreApplication::applicationDirPath(); // mimetex должен лежать там же где и mytetra
    ExecuteCommand exCommand;


    if(exCommand.getOsFamily()=="unix") {
        mimetexBinaryName="./"+mimetexBinaryName;
        chDirCommand="cd "+mimetexPath+" ; ";
    }

    if(exCommand.getOsFamily()=="windows") {
        mimetexBinaryName+=".exe";
        chDirCommand="chdir /D "+mimetexPath+" & ";
    }

    // QString command=chDirCommand+mimetexBinaryName+" -e "+iFileName+QChar::fromLatin1(34)+iMathExpression+QChar::fromLatin1(34);
    QString command=chDirCommand+mimetexBinaryName+" -e "+iFileName+" "+QChar::fromLatin1(34)+iMathExpression+QChar::fromLatin1(34);

    qDebug() << "Command for create math expression picture: " << command;

    exCommand.setCommand(command);
    exCommand.runSimple();
}


void MathExpressionFormatter::insertMathExpressionToTextArea(QString iMathExpressionText)
{
    QString tempFileName=QDir::tempPath()+"/"+getUniqueId()+".gif";

    qDebug() << "Formula code: " << iMathExpressionText;
    qDebug() << "Formula temporary file name: " << tempFileName;

    createGifFromMathExpression( iMathExpressionText, tempFileName );

    bool isSuccess=false;

    // Если картинка сформировалась (подумать, возможно надо вставлсять картинку в текст через форматтер ImageFormatter)
    if ( QFile::exists(tempFileName) ) {
        // Временная картинка загружается из файла в память
        QImage image = QImageReader(tempFileName).read();

        // И сразу удаляется в корзину
        DiskHelper::removeFileToTrash( tempFileName );

        // Если картинка была нормально загружена из файла
        if( !image.isNull() ) {
            // Картинка в памяти запоминает исходный код формулы
            image.setText("Description", imageDescriptionPrefix+iMathExpressionText);

            // Внутреннее имя картинки
            QString imageName=getUniqueImageName();

            // Картинка добавляется в хранилище документа под своим внутренним именем
            textArea->document()->addResource(QTextDocument::ImageResource, QUrl(imageName), image );

            // Создается описание форматированной картинки
            // QTextImageFormat imageFormat;
            // imageFormat.setName(link.toString());

            // Картинка вставляется в текст
            QTextCursor cursor=textArea->textCursor();
            cursor.insertImage(imageName);

            // Ставится пометка что текст записи был изменен
            textArea->document()->setModified(true);

            isSuccess=true;
        }
    }

    if(!isSuccess) {
        QMessageBox msgBox;
        msgBox.setText(tr("Error while parse TeX syntax"));
        msgBox.setInformativeText("Can't generate math expression picture");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.exec();
    }
}
