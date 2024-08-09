#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QDialog>
#include <QProcess>

class SimpleShutDownDialog : public QWidget {
    Q_OBJECT

public:
    SimpleShutDownDialog() {
        setWindowTitle("Simple ShutDown Dialog");
        setWindowIcon(QIcon(":/icons/ssdd-icon.png"));
        setWindowFlags(Qt::Dialog);
        setWindowModality(Qt::ApplicationModal);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        QGridLayout *gridLayout = new QGridLayout;

        const QStringList labels = {
            "Logout", "Reboot", "Shutdown", "Switch User",
            "Suspend", "Hibernate", "About", "Exit"
        };
        const QStringList icons = {
            "system-log-out", "view-refresh", "system-shutdown", "system-users",
            "media-playback-pause", "media-playback-stop", "help-about", "application-exit"
        };
        const QStringList commands = {
            "openbox --exit", "systemctl reboot", "systemctl poweroff", "dm-tool switch-to-greeter",
            "systemctl suspend", "systemctl hibernate", "about", "exit"
        };

        for (int i = 0; i < labels.size(); ++i) {
            QPushButton *button = new QPushButton;
            QVBoxLayout *vbox = new QVBoxLayout;
            QLabel *iconLabel = new QLabel;
            iconLabel->setPixmap(QIcon::fromTheme(icons[i]).pixmap(48));
            QLabel *textLabel = new QLabel(labels[i]);
            vbox->addWidget(iconLabel, 0, Qt::AlignHCenter);
            vbox->addWidget(textLabel, 0, Qt::AlignHCenter);
            button->setLayout(vbox);

            connect(button, &QPushButton::clicked, this, [this, commands, i]() {
                onButtonClicked(commands[i]);
            });

            gridLayout->addWidget(button, i / 4, i % 4);
        }

        mainLayout->addLayout(gridLayout);
        setLayout(mainLayout);
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Escape) {
            QApplication::quit();
        } else {
            QWidget::keyPressEvent(event);
        }
    }

private slots:
    void onButtonClicked(const QString &command) {
        if (command == "about") {
            showAboutDialog();
        } else if (command == "exit") {
            QApplication::quit();
        } else {
            showConfirmationDialog(command);
        }
    }

    void showAboutDialog() {
        QMessageBox aboutBox;
        aboutBox.setWindowTitle("About Simple ShutDown Dialog");
        aboutBox.setIconPixmap(QPixmap(":/icons/ssdd-icon.png").scaled(250, 250, Qt::KeepAspectRatio));
        aboutBox.setText(
            "<b>About Simple ShutDown Dialog</b><br><br>"
            "<b>Version:</b> 1.3<br>"
            "<b>Author:</b> kekePower<br>"
            "<b>URL:</b> <a href=\"https://git.kekepower.com/kekePower/ssdd\">https://git.kekepower.com/kekePower/ssdd</a><br>"
            "<b>Description:</b> This is a simple Shutdown Dialog for Openbox."
        );
        aboutBox.exec();
    }

    void showConfirmationDialog(const QString &command) {
        QMessageBox confirmBox;
        confirmBox.setWindowTitle("Confirmation");
        confirmBox.setText(QString("Are you sure you want to %1?").arg(command));
        confirmBox.setIcon(QMessageBox::Warning);
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        if (confirmBox.exec() == QMessageBox::Yes) {
            QProcess::startDetached(command);
        }
    }
};

#include "ssdd-qt.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    SimpleShutDownDialog dialog;
    dialog.show();

    return app.exec();
}
