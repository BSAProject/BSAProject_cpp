/********************************************************************************
** Form generated from reading UI file 'gamma.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GAMMA_H
#define UI_GAMMA_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include "GraphicWidget.h"
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE

class Ui_grav
{
public:
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *pushButton_2;
    QPushButton *pushButton;
    QLabel *label_3;
    QComboBox *comboBox_2;
    QLabel *label_2;
    QComboBox *comboBox;
    QLabel *label_8;
    QDoubleSpinBox *doubleSpinBox;
    QLabel *label;
    QSpinBox *spinBox;
    QLabel *label_7;
    QSpinBox *spinBox_2;
    QSpacerItem *horizontalSpacer_2;
    QCheckBox *checkBox;
    QCheckBox *checkBox_2;
    QSpacerItem *horizontalSpacer;
    QPushButton *pushButton_3;
    QToolButton *tbHelp;
    QHBoxLayout *horizontalLayout;
    QLabel *label_4;
    QLineEdit *leData;
    QToolButton *pbSelectFile;
    QLabel *label_9;
    QLineEdit *LineEdit;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_5;
    QLineEdit *leHR;
    QToolButton *pbSelectHR;
    QSpacerItem *horizontalSpacer_4;
    QLabel *label_13;
    QLineEdit *LineEdit_fileIni;
    QToolButton *selectIniFile;
    QPushButton *pushButton_4;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_6;
    QLineEdit *leDir;
    QToolButton *toolButton_3;
    QSpacerItem *horizontalSpacer_3;
    QLabel *label_12;
    QDoubleSpinBox *doubleSpinBox_4;
    QLabel *label_11;
    QDoubleSpinBox *doubleSpinBox_3;
    QLabel *label_10;
    QDoubleSpinBox *doubleSpinBox_2;
    QVBoxLayout *verticalLayout;
    QCustomPlot *widget;
    QSpacerItem *verticalSpacer;
    GraphicWidget *widget_2;

    void setupUi(QDialog *grav)
    {
        if (grav->objectName().isEmpty())
            grav->setObjectName(QStringLiteral("grav"));
        grav->resize(1020, 1210);
        verticalLayout_2 = new QVBoxLayout(grav);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(1);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        pushButton_2 = new QPushButton(grav);
        pushButton_2->setObjectName(QStringLiteral("pushButton_2"));

        horizontalLayout_3->addWidget(pushButton_2);

        pushButton = new QPushButton(grav);
        pushButton->setObjectName(QStringLiteral("pushButton"));

        horizontalLayout_3->addWidget(pushButton);

        label_3 = new QLabel(grav);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setLayoutDirection(Qt::LeftToRight);
        label_3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_3->addWidget(label_3);

        comboBox_2 = new QComboBox(grav);
        comboBox_2->setObjectName(QStringLiteral("comboBox_2"));
        comboBox_2->setMinimumSize(QSize(100, 0));
        comboBox_2->setEditable(false);

        horizontalLayout_3->addWidget(comboBox_2);

        label_2 = new QLabel(grav);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setMinimumSize(QSize(0, 0));
        label_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_3->addWidget(label_2);

        comboBox = new QComboBox(grav);
        comboBox->setObjectName(QStringLiteral("comboBox"));
        comboBox->setMinimumSize(QSize(100, 0));
        comboBox->setEditable(false);

        horizontalLayout_3->addWidget(comboBox);

        label_8 = new QLabel(grav);
        label_8->setObjectName(QStringLiteral("label_8"));

        horizontalLayout_3->addWidget(label_8);

        doubleSpinBox = new QDoubleSpinBox(grav);
        doubleSpinBox->setObjectName(QStringLiteral("doubleSpinBox"));
        doubleSpinBox->setMaximum(3599.99);
        doubleSpinBox->setSingleStep(10);
        doubleSpinBox->setValue(0);

        horizontalLayout_3->addWidget(doubleSpinBox);

        label = new QLabel(grav);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout_3->addWidget(label);

        spinBox = new QSpinBox(grav);
        spinBox->setObjectName(QStringLiteral("spinBox"));
        spinBox->setMinimum(1);
        spinBox->setMaximum(3600);
        spinBox->setSingleStep(10);
        spinBox->setValue(240);

        horizontalLayout_3->addWidget(spinBox);

        label_7 = new QLabel(grav);
        label_7->setObjectName(QStringLiteral("label_7"));

        horizontalLayout_3->addWidget(label_7);

        spinBox_2 = new QSpinBox(grav);
        spinBox_2->setObjectName(QStringLiteral("spinBox_2"));
        spinBox_2->setMinimum(1);
        spinBox_2->setMaximum(360);
        spinBox_2->setValue(15);

        horizontalLayout_3->addWidget(spinBox_2);

        horizontalSpacer_2 = new QSpacerItem(5, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_2);

        checkBox = new QCheckBox(grav);
        checkBox->setObjectName(QStringLiteral("checkBox"));

        horizontalLayout_3->addWidget(checkBox);

        checkBox_2 = new QCheckBox(grav);
        checkBox_2->setObjectName(QStringLiteral("checkBox_2"));

        horizontalLayout_3->addWidget(checkBox_2);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer);

        pushButton_3 = new QPushButton(grav);
        pushButton_3->setObjectName(QStringLiteral("pushButton_3"));
        pushButton_3->setCheckable(true);
        pushButton_3->setChecked(false);

        horizontalLayout_3->addWidget(pushButton_3);

        tbHelp = new QToolButton(grav);
        tbHelp->setObjectName(QStringLiteral("tbHelp"));

        horizontalLayout_3->addWidget(tbHelp);

        horizontalLayout_3->setStretch(0, 1);
        horizontalLayout_3->setStretch(2, 1);
        horizontalLayout_3->setStretch(3, 2);
        horizontalLayout_3->setStretch(4, 1);
        horizontalLayout_3->setStretch(5, 2);

        verticalLayout_2->addLayout(horizontalLayout_3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(1);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label_4 = new QLabel(grav);
        label_4->setObjectName(QStringLiteral("label_4"));

        horizontalLayout->addWidget(label_4);

        leData = new QLineEdit(grav);
        leData->setObjectName(QStringLiteral("leData"));

        horizontalLayout->addWidget(leData);

        pbSelectFile = new QToolButton(grav);
        pbSelectFile->setObjectName(QStringLiteral("pbSelectFile"));

        horizontalLayout->addWidget(pbSelectFile);

        label_9 = new QLabel(grav);
        label_9->setObjectName(QStringLiteral("label_9"));

        horizontalLayout->addWidget(label_9);

        LineEdit = new QLineEdit(grav);
        LineEdit->setObjectName(QStringLiteral("LineEdit"));
        LineEdit->setReadOnly(false);

        horizontalLayout->addWidget(LineEdit);


        verticalLayout_2->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(1);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        label_5 = new QLabel(grav);
        label_5->setObjectName(QStringLiteral("label_5"));

        horizontalLayout_2->addWidget(label_5);

        leHR = new QLineEdit(grav);
        leHR->setObjectName(QStringLiteral("leHR"));

        horizontalLayout_2->addWidget(leHR);

        pbSelectHR = new QToolButton(grav);
        pbSelectHR->setObjectName(QStringLiteral("pbSelectHR"));

        horizontalLayout_2->addWidget(pbSelectHR);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_4);

        label_13 = new QLabel(grav);
        label_13->setObjectName(QStringLiteral("label_13"));

        horizontalLayout_2->addWidget(label_13);

        LineEdit_fileIni = new QLineEdit(grav);
        LineEdit_fileIni->setObjectName(QStringLiteral("LineEdit_fileIni"));

        horizontalLayout_2->addWidget(LineEdit_fileIni);

        selectIniFile = new QToolButton(grav);
        selectIniFile->setObjectName(QStringLiteral("selectIniFile"));

        horizontalLayout_2->addWidget(selectIniFile);

        pushButton_4 = new QPushButton(grav);
        pushButton_4->setObjectName(QStringLiteral("pushButton_4"));

        horizontalLayout_2->addWidget(pushButton_4);


        verticalLayout_2->addLayout(horizontalLayout_2);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        label_6 = new QLabel(grav);
        label_6->setObjectName(QStringLiteral("label_6"));

        horizontalLayout_4->addWidget(label_6);

        leDir = new QLineEdit(grav);
        leDir->setObjectName(QStringLiteral("leDir"));

        horizontalLayout_4->addWidget(leDir);

        toolButton_3 = new QToolButton(grav);
        toolButton_3->setObjectName(QStringLiteral("toolButton_3"));

        horizontalLayout_4->addWidget(toolButton_3);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_3);

        label_12 = new QLabel(grav);
        label_12->setObjectName(QStringLiteral("label_12"));

        horizontalLayout_4->addWidget(label_12);

        doubleSpinBox_4 = new QDoubleSpinBox(grav);
        doubleSpinBox_4->setObjectName(QStringLiteral("doubleSpinBox_4"));
        doubleSpinBox_4->setMinimum(0.01);
        doubleSpinBox_4->setSingleStep(0.1);
        doubleSpinBox_4->setValue(0.95);

        horizontalLayout_4->addWidget(doubleSpinBox_4);

        label_11 = new QLabel(grav);
        label_11->setObjectName(QStringLiteral("label_11"));

        horizontalLayout_4->addWidget(label_11);

        doubleSpinBox_3 = new QDoubleSpinBox(grav);
        doubleSpinBox_3->setObjectName(QStringLiteral("doubleSpinBox_3"));
        doubleSpinBox_3->setMaximum(10000);
        doubleSpinBox_3->setSingleStep(50);
        doubleSpinBox_3->setValue(100);

        horizontalLayout_4->addWidget(doubleSpinBox_3);

        label_10 = new QLabel(grav);
        label_10->setObjectName(QStringLiteral("label_10"));

        horizontalLayout_4->addWidget(label_10);

        doubleSpinBox_2 = new QDoubleSpinBox(grav);
        doubleSpinBox_2->setObjectName(QStringLiteral("doubleSpinBox_2"));
        doubleSpinBox_2->setMinimum(0.01);
        doubleSpinBox_2->setMaximum(100);
        doubleSpinBox_2->setSingleStep(0.2);
        doubleSpinBox_2->setValue(1);

        horizontalLayout_4->addWidget(doubleSpinBox_2);


        verticalLayout_2->addLayout(horizontalLayout_4);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        widget = new QCustomPlot(grav);
        widget->setObjectName(QStringLiteral("widget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(widget->sizePolicy().hasHeightForWidth());
        widget->setSizePolicy(sizePolicy);
        widget->setMinimumSize(QSize(0, 500));

        verticalLayout->addWidget(widget);

        verticalSpacer = new QSpacerItem(20, 13, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        widget_2 = new GraphicWidget(grav);
        widget_2->setObjectName(QStringLiteral("widget_2"));
        sizePolicy.setHeightForWidth(widget_2->sizePolicy().hasHeightForWidth());
        widget_2->setSizePolicy(sizePolicy);
        widget_2->setMinimumSize(QSize(0, 500));

        verticalLayout->addWidget(widget_2);

        verticalLayout->setStretch(2, 20);

        verticalLayout_2->addLayout(verticalLayout);


        retranslateUi(grav);
        QObject::connect(pushButton_3, SIGNAL(toggled(bool)), widget, SLOT(setVisible(bool)));

        QMetaObject::connectSlotsByName(grav);
    } // setupUi

    void retranslateUi(QDialog *grav)
    {
        grav->setWindowTitle(QApplication::translate("grav", "gamma", 0));
        pushButton_2->setText(QApplication::translate("grav", "\320\227\320\260\320\263\321\200\321\203\320\267\320\270\321\202\321\214", 0));
        pushButton->setText(QApplication::translate("grav", "\320\222\321\213\320\277\320\276\320\273\320\275\320\270\321\202\321\214", 0));
        label_3->setText(QApplication::translate("grav", "\320\232\320\260\320\275\320\260\320\273", 0));
        label_2->setText(QApplication::translate("grav", "\320\237\320\276\320\273\320\276\321\201\320\260", 0));
        label_8->setText(QApplication::translate("grav", "\320\241\321\202\320\260\321\200\321\202 t0(s)", 0));
        label->setText(QApplication::translate("grav", "\320\237\320\276\321\200\321\206\320\270\321\217", 0));
        label_7->setText(QApplication::translate("grav", "\320\247\320\270\321\201\320\273\320\276 \320\277\320\276\321\200.", 0));
        checkBox->setText(QApplication::translate("grav", "\320\241\320\264\320\262\320\270\320\263", 0));
        checkBox_2->setText(QApplication::translate("grav", "\320\223\321\200\320\260\321\204\320\270\320\272", 0));
        pushButton_3->setText(QApplication::translate("grav", "\320\221\320\276\320\273\321\214\321\210\320\265", 0));
        tbHelp->setText(QApplication::translate("grav", "?", 0));
        label_4->setText(QApplication::translate("grav", "\320\224\320\260\320\275\320\275\321\213\320\265:", 0));
        pbSelectFile->setText(QApplication::translate("grav", "...", 0));
        label_9->setText(QApplication::translate("grav", "\320\242\320\265\321\201\321\202\321\213:", 0));
        label_5->setText(QApplication::translate("grav", "\320\232\320\260\320\273\320\270\320\261\321\200\320\276\320\262\320\272\320\260:", 0));
        pbSelectHR->setText(QApplication::translate("grav", "...", 0));
        label_13->setText(QApplication::translate("grav", "File_ini:", 0));
        selectIniFile->setText(QApplication::translate("grav", "...", 0));
        pushButton_4->setText(QApplication::translate("grav", "Proc.", 0));
        label_6->setText(QApplication::translate("grav", "\320\232\320\260\321\202\320\260\320\273\320\276\320\263 \321\201\320\276\321\205\321\200\320\260\320\275\320\265\320\275\320\270\321\217 \321\204\320\260\320\271\320\273\320\276\320\262:", 0));
        toolButton_3->setText(QApplication::translate("grav", "...", 0));
        label_12->setText(QApplication::translate("grav", "Mult.x", 0));
        label_11->setText(QApplication::translate("grav", "dT_scale", 0));
        label_10->setText(QApplication::translate("grav", "Y *", 0));
    } // retranslateUi

};

namespace Ui {
    class grav: public Ui_grav {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GAMMA_H
