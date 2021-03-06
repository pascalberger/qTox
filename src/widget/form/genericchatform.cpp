/*
    Copyright © 2014-2015 by The qTox Project Contributors

    This file is part of qTox, a Qt-based graphical interface for Tox.

    qTox is libre software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    qTox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with qTox.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "genericchatform.h"

#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QClipboard>
#include <QToolButton>

#include "src/chatlog/chatlog.h"
#include "src/chatlog/content/timestamp.h"
#include "src/core/core.h"
#include "src/friend.h"
#include "src/friendlist.h"
#include "src/group.h"
#include "src/grouplist.h"
#include "src/persistence/settings.h"
#include "src/persistence/smileypack.h"
#include "src/video/genericnetcamview.h"
#include "src/widget/contentdialog.h"
#include "src/widget/contentlayout.h"
#include "src/widget/emoticonswidget.h"
#include "src/widget/maskablepixmapwidget.h"
#include "src/widget/style.h"
#include "src/widget/tool/chattextedit.h"
#include "src/widget/tool/croppinglabel.h"
#include "src/widget/tool/flyoutoverlaywidget.h"
#include "src/widget/translator.h"
#include "src/widget/widget.h"

/**
 * @class GenericChatForm
 * @brief Parent class for all chatforms. It's provide the minimum required UI
 * elements and methods to work with chat messages.
 *
 * TODO: reword
 * @var GenericChatForm::historyBaselineDate
 * @brief Used by HistoryKeeper to load messages from t to historyBaselineDate
 *        (excluded)
 */

GenericChatForm::GenericChatForm(QWidget *parent)
  : QWidget(parent, Qt::Window)
  , audioInputFlag(false)
  , audioOutputFlag(false)
{
    curRow = 0;
    headWidget = new QWidget();

    nameLabel = new CroppingLabel();
    nameLabel->setObjectName("nameLabel");
    nameLabel->setMinimumHeight(Style::getFont(Style::Medium).pixelSize());
    nameLabel->setEditable(true);
    nameLabel->setTextFormat(Qt::PlainText);

    avatar = new MaskablePixmapWidget(this, QSize(40,40), ":/img/avatar_mask.svg");
    QHBoxLayout *mainFootLayout = new QHBoxLayout(),
                *headLayout = new QHBoxLayout();

    QVBoxLayout *mainLayout = new QVBoxLayout(),
                *footButtonsSmall = new QVBoxLayout(),
                *micButtonsLayout = new QVBoxLayout();
                headTextLayout = new QVBoxLayout();

    QGridLayout *buttonsLayout = new QGridLayout();

    chatWidget = new ChatLog(this);
    chatWidget->setBusyNotification(ChatMessage::createBusyNotification());

    // settings
    const Settings& s = Settings::getInstance();
    connect(&s, &Settings::emojiFontPointSizeChanged,
            chatWidget, &ChatLog::forceRelayout);

    msgEdit = new ChatTextEdit();
    // TODO: make it work without restart
    msgEdit->setCurrentFont(s.getChatMessageFont());

    sendButton = new QPushButton();
    emoteButton = new QPushButton();

    // Setting the sizes in the CSS doesn't work (glitch with high DPIs)
    fileButton = new QPushButton();
    screenshotButton = new QPushButton;
    callButton = new QPushButton();
    callButton->setFixedSize(50,40);

    videoButton = new QPushButton();
    videoButton->setFixedSize(50,40);

    volButton = new QToolButton();
    volButton->setFixedSize(22, 18);

    micButton = new QToolButton();
    micButton->setFixedSize(22, 18);
    // TODO: Make updateCallButtons (see ChatForm) abstract
    //       and call here to set tooltips.

    fileFlyout = new FlyoutOverlayWidget;
    QHBoxLayout *fileLayout = new QHBoxLayout(fileFlyout);
    fileLayout->addWidget(screenshotButton);
    fileLayout->setContentsMargins(0, 0, 0, 0);

    footButtonsSmall->setSpacing(2);
    fileLayout->setSpacing(0);
    fileLayout->setMargin(0);

    msgEdit->setStyleSheet(Style::getStylesheet(":/ui/msgEdit/msgEdit.css"));
    msgEdit->setFixedHeight(50);
    msgEdit->setFrameStyle(QFrame::NoFrame);

    sendButton->setStyleSheet(Style::getStylesheet(":/ui/sendButton/sendButton.css"));
    fileButton->setStyleSheet(Style::getStylesheet(":/ui/fileButton/fileButton.css"));
    screenshotButton->setStyleSheet(Style::getStylesheet(":/ui/screenshotButton/screenshotButton.css"));
    emoteButton->setStyleSheet(Style::getStylesheet(":/ui/emoteButton/emoteButton.css"));

    callButton->setObjectName("green");
    callButton->setStyleSheet(Style::getStylesheet(":/ui/callButton/callButton.css"));

    videoButton->setObjectName("green");
    videoButton->setStyleSheet(Style::getStylesheet(":/ui/videoButton/videoButton.css"));

    QString volButtonStylesheet = Style::getStylesheet(":/ui/volButton/volButton.css");
    volButton->setObjectName("grey");
    volButton->setStyleSheet(volButtonStylesheet);

    QString micButtonStylesheet = Style::getStylesheet(":/ui/micButton/micButton.css");
    micButton->setObjectName("grey");
    micButton->setStyleSheet(micButtonStylesheet);

    setLayout(mainLayout);

    bodySplitter = new QSplitter(Qt::Vertical, this);
    connect(bodySplitter, &QSplitter::splitterMoved, this, &GenericChatForm::onSplitterMoved);

    QWidget* contentWidget = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->addWidget(chatWidget);
    contentLayout->addLayout(mainFootLayout);
    bodySplitter->addWidget(contentWidget);

    mainLayout->addWidget(bodySplitter);
    mainLayout->setMargin(0);

    footButtonsSmall->addWidget(emoteButton);
    footButtonsSmall->addWidget(fileButton);

    mainFootLayout->addWidget(msgEdit);
    mainFootLayout->addLayout(footButtonsSmall);
    mainFootLayout->addSpacing(5);
    mainFootLayout->addWidget(sendButton);
    mainFootLayout->setSpacing(0);

    headTextLayout->addStretch();
    headTextLayout->addWidget(nameLabel);
    headTextLayout->addStretch();

    micButtonsLayout->setSpacing(4);
    micButtonsLayout->addWidget(micButton, Qt::AlignTop | Qt::AlignRight);
    micButtonsLayout->addWidget(volButton, Qt::AlignTop | Qt::AlignRight);

    buttonsLayout->addLayout(micButtonsLayout, 0, 0, 2, 1, Qt::AlignTop | Qt::AlignRight);
    buttonsLayout->addWidget(callButton, 0, 1, 2, 1, Qt::AlignTop);
    buttonsLayout->addWidget(videoButton, 0, 2, 2, 1, Qt::AlignTop);
    buttonsLayout->setVerticalSpacing(0);
    buttonsLayout->setHorizontalSpacing(4);

    headLayout->addWidget(avatar);
    headLayout->addSpacing(5);
    headLayout->addLayout(headTextLayout);
    headLayout->addLayout(buttonsLayout);

    headWidget->setLayout(headLayout);

    //Fix for incorrect layouts on OS X as per
    //https://bugreports.qt-project.org/browse/QTBUG-14591
    sendButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    fileButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    screenshotButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    emoteButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    micButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    volButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    callButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    videoButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);

    menu.addActions(chatWidget->actions());
    menu.addSeparator();
    saveChatAction = menu.addAction(QIcon::fromTheme("document-save"),
                                    QString(), this, SLOT(onSaveLogClicked()));
    clearAction = menu.addAction(QIcon::fromTheme("edit-clear"),
                                 QString(), this, SLOT(clearChatArea(bool)));

    quoteAction = menu.addAction(QIcon(),
                                 QString(), this, SLOT(quoteSelectedText()));

    copyLinkAction = menu.addAction(QIcon(),
                                    QString(), this, SLOT(copyLink()));

    menu.addSeparator();

    connect(emoteButton, &QPushButton::clicked,
            this, &GenericChatForm::onEmoteButtonClicked);
    connect(chatWidget, &ChatLog::customContextMenuRequested,
            this, &GenericChatForm::onChatContextMenuRequested);

    new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_L, this, SLOT(clearChatArea()));
    new QShortcut(Qt::ALT + Qt::Key_Q, this, SLOT(quoteSelectedText()));

    chatWidget->setStyleSheet(Style::getStylesheet(":/ui/chatArea/chatArea.css"));
    headWidget->setStyleSheet(Style::getStylesheet(":/ui/chatArea/chatHead.css"));

    fileFlyout->setFixedSize(24, 24);
    fileFlyout->setParent(this);
    fileButton->installEventFilter(this);
    fileFlyout->installEventFilter(this);

    retranslateUi();
    Translator::registerHandler(std::bind(&GenericChatForm::retranslateUi, this), this);

    netcam = nullptr;
}

GenericChatForm::~GenericChatForm()
{
    Translator::unregister(this);
}

void GenericChatForm::adjustFileMenuPosition()
{
    QPoint pos = fileButton->mapTo(bodySplitter, QPoint());
    QSize size = fileFlyout->size();
    fileFlyout->move(pos.x() - size.width(), pos.y());
}

void GenericChatForm::showFileMenu()
{
    if (!fileFlyout->isShown() && !fileFlyout->isBeingShown()) {
        adjustFileMenuPosition();
    }

    fileFlyout->animateShow();
}

void GenericChatForm::hideFileMenu()
{
    if (fileFlyout->isShown() || fileFlyout->isBeingShown())
        fileFlyout->animateHide();
}

bool GenericChatForm::isEmpty()
{
    return chatWidget->isEmpty();
}

ChatLog *GenericChatForm::getChatLog() const
{
    return chatWidget;
}

QDate GenericChatForm::getLatestDate() const
{
    ChatLine::Ptr chatLine = chatWidget->getLatestLine();

    if (chatLine)
    {
        Timestamp* timestamp = qobject_cast<Timestamp*>(chatLine->getContent(2));

        if (timestamp)
            return timestamp->getTime().date();
        else
            return QDate::currentDate();
    }

    return QDate();
}

void GenericChatForm::setName(const QString &newName)
{
    nameLabel->setText(newName);
    nameLabel->setToolTip(Qt::convertFromPlainText(newName, Qt::WhiteSpaceNormal)); // for overlength names
}

void GenericChatForm::show(ContentLayout* contentLayout)
{
    contentLayout->mainContent->layout()->addWidget(this);
    contentLayout->mainHead->layout()->addWidget(headWidget);
    headWidget->show();
    QWidget::show();
}

void GenericChatForm::showEvent(QShowEvent *)
{
    msgEdit->setFocus();
}

bool GenericChatForm::event(QEvent* e)
{
    // If the user accidentally starts typing outside of the msgEdit, focus it automatically
    if (e->type() == QEvent::KeyRelease && !msgEdit->hasFocus())
    {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);
        if ((ke->modifiers() == Qt::NoModifier || ke->modifiers() == Qt::ShiftModifier)
                && !ke->text().isEmpty())
            msgEdit->setFocus();
    }
    return QWidget::event(e);
}

void GenericChatForm::onChatContextMenuRequested(QPoint pos)
{
    QWidget* sender = static_cast<QWidget*>(QObject::sender());
    pos = sender->mapToGlobal(pos);

    // If we right-clicked on a link, give the option to copy it
    bool clickedOnLink = false;
    Text* clickedText = qobject_cast<Text*>(chatWidget->getContentFromGlobalPos(pos));
    if (clickedText)
    {
        QPointF scenePos = chatWidget->mapToScene(chatWidget->mapFromGlobal(pos));
        QString linkTarget = clickedText->getLinkAt(scenePos);
        if (!linkTarget.isEmpty())
        {
            clickedOnLink = true;
            copyLinkAction->setData(linkTarget);
        }
    }
    copyLinkAction->setVisible(clickedOnLink);

    menu.exec(pos);
}

ChatMessage::Ptr GenericChatForm::addMessage(const ToxPk& author, const QString &message, bool isAction,
                                             const QDateTime &datetime, bool isSent)
{
    const Core* core = Core::getInstance();
    bool authorIsActiveProfile = author == core->getSelfId().getPublicKey();
    QString authorStr = authorIsActiveProfile ? core->getUsername() : resolveToxId(author);


    if (getLatestDate() != QDate::currentDate())
    {
        const Settings& s = Settings::getInstance();
        QString dateText = QDate::currentDate().toString(s.getDateFormat());
        addSystemInfoMessage(dateText, ChatMessage::INFO, QDateTime());
    }

    ChatMessage::Ptr msg;
    if (isAction)
    {
        msg = ChatMessage::createChatMessage(authorStr, message, ChatMessage::ACTION, authorIsActiveProfile);
        previousId = ToxPk();
    }
    else
    {
        msg = ChatMessage::createChatMessage(authorStr, message, ChatMessage::NORMAL, authorIsActiveProfile);
        if ( (author == previousId) && (prevMsgDateTime.secsTo(QDateTime::currentDateTime()) < getChatLog()->repNameAfter) )
            msg->hideSender();

        previousId = author;
        prevMsgDateTime = QDateTime::currentDateTime();
    }

    insertChatMessage(msg);

    if (isSent)
        msg->markAsSent(datetime);

    return msg;
}

ChatMessage::Ptr GenericChatForm::addSelfMessage(const QString &message, bool isAction, const QDateTime &datetime, bool isSent)
{
    return addMessage(Core::getInstance()->getSelfId().getPublicKey(), message, isAction, datetime, isSent);
}

void GenericChatForm::addAlertMessage(const ToxPk &author, QString message, QDateTime datetime)
{
    QString authorStr = resolveToxId(author);
    bool isSelf = author == Core::getInstance()->getSelfId().getPublicKey();
    ChatMessage::Ptr msg = ChatMessage::createChatMessage(authorStr, message, ChatMessage::ALERT, isSelf, datetime);
    insertChatMessage(msg);

    if ((author == previousId) && (prevMsgDateTime.secsTo(QDateTime::currentDateTime()) < getChatLog()->repNameAfter))
        msg->hideSender();

    previousId = author;
    prevMsgDateTime = QDateTime::currentDateTime();
}

void GenericChatForm::onEmoteButtonClicked()
{
    // don't show the smiley selection widget if there are no smileys available
    if (SmileyPack::getInstance().getEmoticons().empty())
        return;

    EmoticonsWidget widget;
    connect(&widget, SIGNAL(insertEmoticon(QString)), this, SLOT(onEmoteInsertRequested(QString)));
    widget.installEventFilter(this);

    QWidget* sender = qobject_cast<QWidget*>(QObject::sender());
    if (sender)
    {
        QPoint pos = -QPoint(widget.sizeHint().width() / 2, widget.sizeHint().height()) - QPoint(0, 10);
        widget.exec(sender->mapToGlobal(pos));
    }
}

void GenericChatForm::onEmoteInsertRequested(QString str)
{
    // insert the emoticon
    QWidget* sender = qobject_cast<QWidget*>(QObject::sender());
    if (sender)
        msgEdit->insertPlainText(str);

    msgEdit->setFocus(); // refocus so that we can continue typing
}

void GenericChatForm::onSaveLogClicked()
{
    QString path = QFileDialog::getSaveFileName(0, tr("Save chat log"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QString plainText;
    auto lines = chatWidget->getLines();
    for (ChatLine::Ptr l : lines)
    {
        Timestamp* rightCol = qobject_cast<Timestamp*>(l->getContent(2));

        if (!rightCol)
            break;

        ChatLineContent* middleCol = l->getContent(1);
        ChatLineContent* leftCol = l->getContent(0);

        QString timestamp = rightCol->getTime().isNull() ? tr("Not sent") : rightCol->getText();
        QString nick = leftCol->getText();
        QString msg = middleCol->getText();

        plainText += QString("[%2] %1\n%3\n\n").arg(nick, timestamp, msg);
    }

    file.write(plainText.toUtf8());
    file.close();
}

void GenericChatForm::onCopyLogClicked()
{
    chatWidget->copySelectedText();
}

void GenericChatForm::focusInput()
{
    msgEdit->setFocus();
}

void GenericChatForm::addSystemInfoMessage(const QString &message, ChatMessage::SystemMessageType type, const QDateTime &datetime)
{
    previousId = ToxPk();
    insertChatMessage(ChatMessage::createChatInfoMessage(message, type, datetime));
}

void GenericChatForm::clearChatArea()
{
    clearChatArea(true);
}

void GenericChatForm::clearChatArea(bool notinform)
{
    chatWidget->clear();
    previousId = ToxPk();

    if (!notinform)
        addSystemInfoMessage(tr("Cleared"), ChatMessage::INFO, QDateTime::currentDateTime());

    earliestMessage = QDateTime(); //null
    historyBaselineDate = QDateTime::currentDateTime();

    emit chatAreaCleared();
}

void GenericChatForm::onSelectAllClicked()
{
    chatWidget->selectAll();
}

QString GenericChatForm::resolveToxId(const ToxPk &id)
{
    Friend* f = FriendList::findFriend(id);
    if (f)
        return f->getDisplayedName();

    for (Group *it : GroupList::getAllGroups())
    {
        QString res = it->resolveToxId(id);
        if (res.size())
            return res;
    }

    return QString();
}

void GenericChatForm::insertChatMessage(ChatMessage::Ptr msg)
{
    chatWidget->insertChatlineAtBottom(std::static_pointer_cast<ChatLine>(msg));
    emit messageInserted();
}

void GenericChatForm::hideEvent(QHideEvent* event)
{
    hideFileMenu();
    QWidget::hideEvent(event);
}

void GenericChatForm::resizeEvent(QResizeEvent* event)
{
    adjustFileMenuPosition();
    QWidget::resizeEvent(event);
}

bool GenericChatForm::eventFilter(QObject* object, QEvent* event)
{
    EmoticonsWidget* ev = qobject_cast<EmoticonsWidget*>(object);
    if (ev && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        msgEdit->sendKeyEvent(key);
        msgEdit->setFocus();
        return false;
    }

    if (object != this->fileButton && object != this->fileFlyout)
        return false;

    if (!qobject_cast<QWidget*>(object)->isEnabled())
        return false;

    switch(event->type())
    {
    case QEvent::Enter:
        showFileMenu();
        break;

    case QEvent::Leave:
    {
        QPoint flyPos = fileFlyout->mapToGlobal(QPoint());
        QSize flySize = fileFlyout->size();

        QPoint filePos = fileButton->mapToGlobal(QPoint());
        QSize fileSize = fileButton->size();

        QRect region = QRect(flyPos, flySize).united(QRect(filePos, fileSize));

        if (!region.contains(QCursor::pos()))
            hideFileMenu();

        break;
    }

    case QEvent::MouseButtonPress:
        hideFileMenu();
        break;

    default:
        break;
    }

    return false;
}

void GenericChatForm::onSplitterMoved(int, int)
{
    if (netcam)
        netcam->setShowMessages(bodySplitter->sizes()[1] == 0);
}

void GenericChatForm::onShowMessagesClicked()
{
    if (netcam)
    {
        if (bodySplitter->sizes()[1] == 0)
            bodySplitter->setSizes({1, 1});
        else
            bodySplitter->setSizes({1, 0});

        onSplitterMoved(0, 0);
    }
}

void GenericChatForm::quoteSelectedText()
{
    QString selectedText = chatWidget->getSelectedText();

    if (selectedText.isEmpty())
        return;

    // forming pretty quote text
    // 1. insert "> " to the begining of quote;
    // 2. replace all possible line terminators with "\n> ";
    // 3. append new line to the end of quote.
    QString quote = selectedText;

    quote.insert(0, "> ");
    quote.replace(QRegExp(QString("\r\n|[\r\n\u2028\u2029]")), QString("\n> "));
    quote.append("\n");

    msgEdit->append(quote);
}

/**
 * @brief Callback of GenericChatForm::copyLinkAction
 */
void GenericChatForm::copyLink()
{
    QString linkText = copyLinkAction->data().toString();
    QApplication::clipboard()->setText(linkText);
}

void GenericChatForm::retranslateUi()
{
    QString callObjectName = callButton->objectName();
    QString videoObjectName = videoButton->objectName();

    if (callObjectName == QStringLiteral("green"))
        callButton->setToolTip(tr("Start audio call"));
    else if (callObjectName == QStringLiteral("yellow"))
        callButton->setToolTip(tr("Accept audio call"));
    else if (callObjectName == QStringLiteral("red"))
        callButton->setToolTip(tr("End audio call"));

    if (videoObjectName == QStringLiteral("green"))
        videoButton->setToolTip(tr("Start video call"));
    else if (videoObjectName == QStringLiteral("yellow"))
        videoButton->setToolTip(tr("Accept video call"));
    else if (videoObjectName == QStringLiteral("red"))
        videoButton->setToolTip(tr("End video call"));

    sendButton->setToolTip(tr("Send message"));
    emoteButton->setToolTip(tr("Smileys"));
    fileButton->setToolTip(tr("Send file(s)"));
    screenshotButton->setToolTip(tr("Send a screenshot"));
    saveChatAction->setText(tr("Save chat log"));
    clearAction->setText(tr("Clear displayed messages"));
    quoteAction->setText(tr("Quote selected text"));
    copyLinkAction->setText(tr("Copy link address"));
}

void GenericChatForm::showNetcam()
{
    if (!netcam)
        netcam = createNetcam();

    connect(netcam, &GenericNetCamView::showMessageClicked,
            this, &GenericChatForm::onShowMessagesClicked);

    bodySplitter->insertWidget(0, netcam);
    bodySplitter->setCollapsible(0, false);

    QSize minSize = netcam->getSurfaceMinSize();
    ContentDialog* current = ContentDialog::current();
    if (current)
        current->onVideoShow(minSize);
}

void GenericChatForm::hideNetcam()
{
    if (!netcam)
        return;

    ContentDialog* current = ContentDialog::current();
    if (current)
        current->onVideoHide();

    netcam->close();
    netcam->hide();
    delete netcam;
    netcam = nullptr;
}
