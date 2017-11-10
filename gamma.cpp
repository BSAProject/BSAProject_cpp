#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
//#include <QDebug>
#include <QtGui>

//Самодуров: для ввода-вывода файлов:
#include <QFile> // Подключаем класс QFile
#include <QTextStream> // Подключаем класс QTextStream
#include <omp.h> //Подключаем OpenMP для параллельной работы нескольких ядер процессора

//#include <fstream>
//#include <iostream>
//#include <cstring>

///#include <fstream>

#include "gamma.h"
#include "ui_gamma.h"


QFile fileOut("D:\\DATA\\LOGS\\tmp_1.txt");
// Связываем объект с файлом fileout.txt

QTextStream writeStream(&fileOut); // Создаем объект класса QTextStream
 // и передаем ему адрес объекта fileOut
QString str1;

// функции из книги "Астрономия на персональном компьютере"

//------------------------------------------------------------------------------
//
// Ddd: Conversion of angular degrees, minutes and seconds of arc to decimal
//   representation of an angle
//
// Input:
//
//   D        Angular degrees
//   M        Minutes of arc
//   S        Seconds of arc
//
// <return>:  Angle in decimal representation
//
//------------------------------------------------------------------------------
double Ddd(int D, int M, double S)
{
  //
  // Variables
  //
  double sign;


  if ( (D<0) || (M<0) || (S<0) ) sign = -1.0; else sign = 1.0;

  return  sign * ( fabs(D)+fabs(M)/60.0+fabs(S)/3600.0 );
}

//------------------------------------------------------------------------------
//
// Mjd: Modified Julian Date from calendar date and time
//
// Input:
//
//   Year      Calendar date components
//   Month
//   Day
//   Hour      Time components (optional)
//   Min
//   Sec
//
// <return>:   Modified Julian Date
//
//------------------------------------------------------------------------------
double Mjd ( int Year, int Month, int Day,
             int Hour, int Min, double Sec )
{
  //
  // Variables
  //
  long    MjdMidnight;
  double  FracOfDay;
  int     b;


  if (Month<=2) { Month+=12; --Year;}

  if ( (10000L*Year+100L*Month+Day) <= 15821004L )
    b = -2 + ((Year+4716)/4) - 1179;     // Julian calendar
  else
    b = (Year/400)-(Year/100)+(Year/4);  // Gregorian calendar

  MjdMidnight = 365L*Year - 679004L + b + int(30.6001*(Month+1)) + Day;
  FracOfDay   = Ddd(Hour,Min,Sec) / 24.0;

  return MjdMidnight + FracOfDay;
}

//------------------------------------------------------------------------------
//
// GMST: Greenwich mean sidereal time
//
// Input:
//
//   MJD       Time as Modified Julian Date
//
// <return>:   GMST in [rad]
//
//------------------------------------------------------------------------------
double GMST (double MJD)
{
  //
  // Constants
  //
  const double Secs = 86400.0;        // Seconds per day


  //
  // Variables
  //
  double MJD_0, UT, T_0, T, gmst;


  MJD_0 = floor ( MJD );
  UT    = Secs*(MJD-MJD_0);     // [s]
  T_0   = (MJD_0-51544.5)/36525.0;
  T     = (MJD  -51544.5)/36525.0;

  gmst  = 24110.54841 + 8640184.812866*T_0 + 1.0027379093*UT
          + (0.093104-6.2e-6*T)*T*T;      // [sec]

  return gmst;

  //return  (2.0*M_PI/Secs)*std::fmod(gmst,Secs);   // [Rad]
}

//-------------------------------------------------------

grav::grav(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::grav)
{
    ui->setupUi(this);

    ui->widget_2->hide();
    ui->widget->hide();

    layout()->setSizeConstraint(QLayout::SetFixedSize);

    ui->widget->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes);

    connect(ui->widget, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));
    connect(ui->widget, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress(QMouseEvent*)));
    connect(ui->widget, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel(QWheelEvent*)));

    connect(ui->widget->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(setRange(QCPRange)));
}

grav::~grav()
{
    delete ui;
}

void grav::on_pbSelectFile_clicked()
{
    chooseFile();
}

void grav::chooseFile()
{
    QString filter;
    if (sender()->objectName() == ui->pbSelectFile->objectName())
        filter = "(*.pnt *.pnthr)";
    else if (sender()->objectName() == ui->pbSelectHR->objectName())
        filter = "(*.txt)";

    QString nameFile = QFileDialog::getOpenFileName(0, "Open Dialog", "", filter);

    if (nameFile.isEmpty())
        return;

    if (sender()->objectName() == ui->pbSelectFile->objectName())
        ui->leData->setText(nameFile);
    else if (sender()->objectName() == ui->pbSelectHR->objectName())
        ui->leHR->setText(nameFile);
}

void grav::on_selectIniFile_clicked()
{
    QString filter;
    filter = "(*.ini)";
    if (ui->selectIniFile->text().isEmpty()) //Не введено имя файла в строку формы
        return;

    name_file_of_ini = QFileDialog::getOpenFileName(0, "Open Dialog", "", filter);

    ui->LineEdit_fileIni->setText(name_file_of_ini);
}


void grav::on_pushButton_2_clicked() //Кнопка с надписью "Загрузить" - слева вверху
//- последовательно грузим  шапку файла данных и файл эквивалентов
{
    if (ui->leData->text().isEmpty()) //Не введено имя файла в строку формы
        return;

    bsaFile.clear();

    bsaFile.name_file_of_datas = ui->leData->text();

    // открывает файл
    std::ifstream qinClientFile(bsaFile.name_file_of_datas.toStdString().c_str(), std::ios::binary);

    // если невозможно открыть файл, выйти из программы
    if (!qinClientFile)
    {
       QMessageBox::information(this, "A Message from the Application", "File could not be opened");
       return;
    }

    unsigned int header_counts = 0;
    QStringList list, list_tmp;
    QString qnext_string;
    QString OutLine_1, date_MSK, time_MSK, date_and_time;   //(Самодуров В.А.)
    std::string next_string;
    std::string tmp_string; //(Самодуров В.А.)
    QDateTime dt; //(Самодуров В.А.)
    QErrorMessage errorMessage; //(Самодуров В.А.)

    while(getline(qinClientFile, next_string, '\n'))
    {
        qnext_string = QString::fromStdString(next_string);


        switch(++header_counts)
        {
        case 1:
            bsaFile.header.numpar = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).toInt();
            break;

        case 2:
            bsaFile.header.source = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed();
            break;

        case 3:
            bsaFile.header.alpha = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed();
            break;

        case 4:
            bsaFile.header.delta = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed();
            break;

        case 5:
            bsaFile.header.fcentral = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).toDouble();
            break;

        case 6:
            bsaFile.header.wb_total = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).toDouble();
            break;

        case 7:
            //bsaFile.header.date_begin = qnext_string.right(qnext_string.length() - qnext_string.indexOf("UTC") - strlen("UTC")).trimmed();
            // Нет, НЕ НАДО брать второе значение - оно есть не во всех файлах. Надо брать МСК, и потом -->UTC
            date_MSK = qnext_string.simplified();
            list_tmp= date_MSK.split(" ", QString::SkipEmptyParts);
            date_MSK=list_tmp.at(1);
            // взяли дату по МСК, потом, в case 8: перейдем -->UTC
            break;

        case 8:
            //bsaFile.header.time_begin = qnext_string.right(qnext_string.length() - qnext_string.indexOf("UTC") - strlen("UTC")).trimmed();
            // Нет, НЕ НАДО брать второе значение - оно есть не во всех файлах. Надо брать МСК, и потом -->UTC
            time_MSK = qnext_string.simplified();
            list_tmp= time_MSK.split(" ", QString::SkipEmptyParts);
            time_MSK=list_tmp.at(1);
            // взяли время по МСК, теперь перейдем -->UTC, вычтя 4 часа
            date_and_time=date_MSK + " " + time_MSK;
            //dt = QDateTime::fromString(date_and_time, "dd.MM.yyyy HH:mm:ss");
            dt = QDateTime::fromString(date_and_time, "dd.MM.yyyy h:mm:ss");
            //h	 -  the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
            dt = dt.addSecs(-14400);
            //Отняли из даты и времени 4 часа "медведевского" московского времени для перехода к UTC
            date_and_time = dt.toString("dd.MM.yyyy hh:mm:ss");

            //Отладочные проверочные сообщения:
            //OutLine_1=date_and_time;
            //ui->LineEdit->setText(OutLine_1);
            //errorMessage.showMessage("Жду тестового нажатия кнопки!");
            //errorMessage.exec();

            list_tmp= date_and_time.split(" ", QString::SkipEmptyParts);

            //Отладочные проверочные сообщения:
            //OutLine_1="Data=" + list_tmp.at(0) + " Time=" + list_tmp.at(1);
            //ui->LineEdit->setText(OutLine_1);
            //errorMessage.showMessage("Жду тестового 2-го нажатия кнопки!");
            //errorMessage.exec();

            bsaFile.header.date_begin = list_tmp.at(0);
            bsaFile.header.time_begin = list_tmp.at(1);
            break;

        case 9:
            bsaFile.header.date_end = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed();
            break;

        case 10:
            bsaFile.header.time_end = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed();
            break;

        case 11:
            list = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed().split(" ");
            foreach (const QString &str, list)
                bsaFile.header.modulus.push_back(str.toDouble());
            break;

        case 12:
            bsaFile.header.tresolution = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).toDouble();
            break;

        case 13:
            bsaFile.header.npoints = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).toInt();
            //Число временнЫх точек данных в файле (каждая такая точка - двумерный массив, содержит обычно
            // данные 48 лучей в 7 или 33 полосах , единичное значение - float формата)
            //Внимание! Число точек в файле вовсе НЕ равно в общем случае 3600 секунд, деленные на tresolution
            // - на самом деле в концовке часа - не хватает точек на отрезке от долей секунды до ~20 секунд
            break;

        case 14:
            bsaFile.header.nbands = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).toInt();
            //Число полос - обычно 6 или 32
            break;

        case 15:
            list = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed().split(" ");
            foreach (const QString &str, list)
                bsaFile.header.wbands.push_back(str.toDouble());
            break;

        case 16:
            list = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed().split(" ");
            foreach (const QString &str, list)
                bsaFile.header.fbands.push_back(str.toDouble());
            break;

        default:
            break;
        }

        if (header_counts >= bsaFile.header.numpar)
            break;
    }

    bsaFile.sizeHeader = qinClientFile.tellg();

    qinClientFile.close();

    int nCountOfModul = (bsaFile.header.nbands+1) * bsaFile.nCanal; // количество лучей в модуле, =8
    long nCountPoint = nCountOfModul * bsaFile.header.modulus.size();
       // ??? количество лучей во всех модулях
       // - НЕТ, это количество значений в одной временнОй точке!
       // - оно равно произведению числа лучей на (число частот +1)
    double double_nCountPoint = nCountPoint;

    unsigned int countsCanal = bsaFile.header.modulus.size() * bsaFile.nCanal; // количество каналов
    npoints = bsaFile.header.npoints;
    //Число временнЫх точек данных в файле

    // заполнение выпадающего списка
    ui->comboBox->clear();
    ui->comboBox->addItem(" (-)");
    for (unsigned int i = 0; i <= bsaFile.header.nbands; i++)
        ui->comboBox->addItem(QString::number(i+1) + (i == bsaFile.header.nbands ? " (усред.)" : (" (" + QString::number(bsaFile.header.fbands[i], 'f', 2) + " МГц)")));

    ui->comboBox_2->clear();
    ui->comboBox_2->addItem(" (-)");
    for (unsigned int i = 0; i < countsCanal; i++)
        //ui->comboBox_2->addItem(QString::number(countsCanal-i) + " ("  /* Было у Торопова - в обратном порядлке нумерация*/
        ui->comboBox_2->addItem(QString::number(i+1) + " ("
                + QString::number(bsaFile.dec[
                      ((bsaFile.name_file_of_datas.indexOf("_n2_", 0, Qt::CaseInsensitive) != -1)
                      ? 6 : 0) + i/bsaFile.nCanal]
                                  [i%bsaFile.nCanal])
                                                      + "°)");
    //(Условие) ? (выражение1):(выражение2)
    // Если условие истинно, то выполняется выражение1, а если ложно, то выражение2.
    //???  != -1 ? 6 : 0   - или 6-я строка берется первой (2-я стойка), или 0-я (1-я стойка)

    // разборка калибровочного файла
    std::ifstream inHRFile(ui->leHR->text().toStdString().c_str(), std::ios::binary);

    if (!inHRFile)
    {
       QMessageBox::information(this, "A Message from the Application", "File HR could not be opened");
       return;
    }


    //QMessageBox::information(this, "A Message from the Application", "IT IS TEST 11111111111111111111111111111111111");
    //return; // выдает во всплывающее окно тестовое сообщение на всю длину

    //qDebug() << QString::fromStdString(bsaFile.header.date_begin) << " " << QString::fromStdString(bsaFile.header.time_begin);


    //std::vector<double> v;
    //объявляем double вектор v (Самодуров В.А.)

    //for (unsigned int i = 0; i <= nCountPoint; i++)
    //    v.insert(0.0);
    //возникает disassmbler строчкой ниже, на втором проходе!
    std::vector<double> v(nCountPoint, 0.0); //Торопов М.О.! - инициализ. 0-ми (Самодуров В.А.)
    // std::vector<double> v(double_nCountPoint, 0.0);

    ui->LineEdit->setText("123321");
    OutLine_1=next_string.c_str();
    //ui->LineEdit->setText(next_string);
    ui->LineEdit->setText(OutLine_1);

    errorMessage.showMessage("Жду тестового нажатия кнопки!");
    errorMessage.exec();

    kalib278.clear();
    kalib2400.clear();

    long Count_string_HRFile=0;

    QRegExp rx("[,\\|{} ]");
    while(getline(inHRFile, next_string, '\n'))
    {
        Count_string_HRFile=Count_string_HRFile+1;
        if(next_string.find('{') == std::string::npos)
            continue; // шапка

        QStringList list = QString::fromStdString(next_string).split(rx, QString::SkipEmptyParts);

        //tmp_string="0=" + list.at(0) + " 1=" + list.at(1) + " 2=" + list.at(2) + " 3=" + list.at(3) + " 4=" + list.at(4);
        //tmp_string=" 1=" + list.at(1).toStdString +
          //       " 2=" + list.at(2).toStdString + " 3=" + list.at(3).toStdString
            //    + " 4=" + list.at(4).toStdString;

       OutLine_1="NN=" + QString::number(Count_string_HRFile) + " l_0=" + list.at(0)
               + " 1=" + list.at(1) + " 2=" + list.at(2) +
               " 3=" + list.at(3) + " 4=" + list.at(4) + " 5=" + list.at(5)
               + " 6=" + list.at(6);
       //tmp_string=" 0=" + list[0].toStdString;
        //tmp_string="0=";
        //OutLine_1=tmp_string.c_str();
        //  OutLine_1= QString::number(nCountPoint);
        //ui->LineEdit->setText(next_string);
        //ui->LineEdit->setText(OutLine_1);
        //   OutLine_1=next_string.c_str();
        //ui->LineEdit->setText(QString::number(nCountPoint));
        ui->LineEdit->setText(OutLine_1);


        //QErrorMessage errorMessage;
        //errorMessage.showMessage("Жду тестового 2-го нажатия кнопки!");
        //errorMessage.exec();



        //ui->LineEdit->setText(nCountPoint);
        for(int i=6; i < list.size(); ++i)
            v[i-6] = list.at(i).toDouble();

        //if(list.at(4).toInt() == 278)
        //    kalib278.insert(std::pair<double, std::vector<double>>(list.at(2).toDouble(), v));
        if(list.at(3).toInt() == 0)
            kalib278.insert(std::pair<double, std::vector<double>>(list.at(2).toDouble(), v));
        //Нет! Правильнее взять признак эквивалента ==0 в _третьей_ ячейке, сразу за MJD
        else if(list.at(3).toInt() == 1)
            kalib2400.insert(std::pair<double, std::vector<double>>(list.at(2).toDouble(), v));

    }
    inHRFile.close(); //Закрываем только что прочтенный нами в нужные структуры файл

    ui->pushButton->setFocus();

}

double grav::calib(double y, double ss, int numberBand, int numberCanal, int otladka_add_1)
//Здесь производлится калибровка данных для указанной точки данных
/*
 Обычно внутри прохода по точкам от 0 до конца ОТРЕЗКА (считанного) данных
for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
                   //Проход по временнЫм точкам
    curve_vec[i] = QPointF(tresolution *i + curTime,
                          //это - Х, отталкиваемся от текущего времени, добавляя i точек, далее - Y,
         которое и считает наша функция
   calib(bsaFile.data_file[size_point * i //- от 0, подразумевая СЧИТАННЫЙ отрезок данных
                           + currentCanal * (bsaFile.header.nbands+1)+ currentBand],//нашли нужный индекс
                           tresolution * i, currentBand, currentCanal)
                            //первое - секунды от начала не файла, А ОТРЕЗКА! второе и третье - полоса и канал

                             /koeff ); - делим снаружи еще на коэфф., но в данном случае это неважно
                      //Это - Y, калибровка данных в каждой точке i - по времени,
                      //currentCanal - номер луча, currentBand - номер полосы
                      //При этом внутри calib накапливается суммой total += y_Kelvin;
 */
{
    double x1, x2, y1, y2, cur_278, cur_2400;
    double cur_mjd_start = mjd_start + ss / 86400;
    int cur_kalib = numberCanal*(bsaFile.header.nbands + 1) + numberBand;

    if ((otladka==1) && (otladka_add_1==1))
     {
        writeStream << "CALB: y,ss,Band,Canal= "
                    << y << ", " << ss << ", "
                    << numberBand << ", " << numberCanal << "; \n mjd_0,cur_mjd,cur_N_klb= "
                    << mjd_start << ", " << cur_mjd_start << ", " << cur_kalib << "\n"; //Запишет в файл параметры
      };//if (otladka==1)

    //Глобальные переменные, определенные в gamma.h и : bsaFile; double mjd_start, curTime;
    // starTime *pStarTime;   int npoints;

    std::pair<std::map<double, std::vector<double>>::iterator,
             std::map<double, std::vector<double>>::iterator> it_278
             = kalib278.equal_range(cur_mjd_start);
     //Что здесь сделано???

    if(it_278.first == it_278.second) //Что здесь и как присваивается???
    {
        y2 = it_278.first->second[cur_kalib];
        x2 = it_278.first->first;
        x1 = (--it_278.first)->first;
        y1 = it_278.first->second[cur_kalib];

        cur_278 = y1 + (y2 - y1) / (x2 - x1) * (cur_mjd_start - x1); // интерполяция значения для калибровки 278K
    }
    else
        cur_278 = it_278.first->second[cur_kalib];


    if ((otladka==1) && (otladka_add_1==1))
     {
        writeStream << "CALB: it_278_y1, it_278_y2, cur_278= "
                    << y1 << ", " << y2  << ", " << cur_278 << "\n"; //Запишет в файл параметры
      };//if (otladka==1)


    std::pair<std::map<double, std::vector<double>>::iterator, std::map<double, std::vector<double>>::iterator> it_2400
                                                                        = kalib2400.equal_range(cur_mjd_start);

    if(it_2400.first == it_2400.second)//Что здесь и как присваивается??? Почему они по умолчанию равны??
    {
        y2 = it_2400.first->second[cur_kalib];
        x2 = it_2400.first->first;
        x1 = (--it_2400.first)->first;
        y1 = it_2400.first->second[cur_kalib];

        cur_2400 = y1 + (y2 - y1) / (x2 - x1) * (cur_mjd_start - x1); // интерполяция значения для калибровки 2400K
    }
    else
        cur_2400 = it_2400.first->second[cur_kalib];

    /* Было, простая модель:
    double One_Kelvin = (cur_2400 - cur_278) / (2400 - 278);//НЕТ!!! Надо считать из файла точные значения для 278...
    double Zero_level =  cur_278 - (One_Kelvin*278);//НЕТ!!! Надо учесть еще шумы приемника!
     */

    double One_Kelvin = (cur_2400 - cur_278) / (2400 - 278);//НЕТ!!! Надо считать из файла точные значения для 278...
    double n=cur_2400/cur_278;
    double T_receiver = (2400 - n*278)/(n-1);
    double Zero_level =  cur_278 - One_Kelvin*(278 + T_receiver);//учтены шумы приемника

    if ((otladka==1) && (otladka_add_1==1))
     {
        writeStream << "c_278,c_2400,One_Kel,n,T_rec,Zero= "
                    << cur_278 << ", " << cur_2400 << ", "
                    << One_Kelvin << ", " << n << ", " << T_receiver << ", " << Zero_level
                    << "\n"; //Запишет в файл параметры
      };//if (otladka==1)

    /*
Малый сигнал-нагрузка , он же – эквивалент температуры окружающей среды:
 Tхолодная нагрузка  он же  T_eq

One_Kelvin       = (Big_signal – Small_signal) / (2400 – 278);
Zero_level         =  Small_signal – (One_Kelvin*278) – (One_Kelvin*T_приемника);

Calb_data_out = (data_input – Zero_level) / One_Kelvin;

T_приемника ~ 200 К  , находится из:
n= Big_signal / Small_signal = (Tприемника + TГШ) / (Tприемника + Tхолодная нагрузка)

T_ приемника =(T_gsh –n* T_eq) / (n-1)

Где:
Small_signal и  Big_signal – соответственно малый калибровочный сигнал (эквивалент антенны) и большой (с ГШ);
Zero_level – уровень 0 шкалы потоков , снимаемых на установке
data_input и Calb_data_out – первоначальные и откалиброванные данные соответственно.

Это – правильная калибровка «по Орешко В.В.»

     */


    double y_Kelvin = (y - Zero_level) / One_Kelvin;
    total += y_Kelvin;//в эту переменную складируем сумму...

    if ((otladka==1) && (otladka_add_1==1))
     {
        writeStream << "y_Kelvin,total= "
                    << y_Kelvin << ", " << total
                    << "\n"; //Запишет в файл параметры
      };//if (otladka==1)

    return y_Kelvin;
}

void grav::on_pushButton_clicked() //Кнопка "Выполнить"
{
    double start_time_in_sec = ui->doubleSpinBox->value();
       // стартовая секунда, с которой надо отрисовывать файлы порций;
    if(bsaFile.name_file_of_datas.isEmpty())
        return;



    bsaFile.header.npoints = npoints;
                   //число точек (временнЫх, т.е. векторов!) в файле данных, в идеале = 36000 или 288000
    int nCountOfModul = (bsaFile.header.nbands+1) * bsaFile.nCanal;
                   // количество (-лучей-) точек данных в одном модуле из 8 лучей - обычно 56 или 264
    long nCountPoint = nCountOfModul * bsaFile.header.modulus.size();
        // количество (-лучей-) точек данных во всех модулях - ТО ЕСТЬ - сколько единичных точек в одной временнОй точке
        //- обычно или 48(лучей)*7(полос)= 336 или 48*33= 1584
    long nCounts = bsaFile.header.npoints*nCountPoint;
              // количество _единичных_ точек данных во всём файле, в идеале 36000*336 или 288000*1584
    unsigned int countsCanal = bsaFile.header.modulus.size() * bsaFile.nCanal;
                           // количество (-каналов-) лучей диаграммы БСА в файле - практически всегда = 48

    // открывает файл
    std::ifstream inClientFile(bsaFile.name_file_of_datas.toStdString().c_str(), std::ios::binary);

    // устанавливаем позицию на начало двоичных данных
    inClientFile.seekg(bsaFile.sizeHeader, std::ios::beg);

    // загрузка данных
    QDateTime start = QDateTime::currentDateTime();

    QProgressDialog* progress_ptr = new QProgressDialog("Формируем файлы ...", "Отмена", 0, 100, this);
    connect(progress_ptr, SIGNAL(canceled()), this, SLOT(on_progress_cancel()));
    progress_ptr->setGeometry(this->geometry().left() + 50, this->geometry().top() + 20, 300, 25);
    progress_ptr->setWindowModality(Qt::ApplicationModal);

    if(ui->widget->isHidden())
        progress_ptr->show();
    else
        widget_2.clearCurves();//рисуем график на экране

    if(ui->checkBox_2->isChecked()) //Чек-бокс с подписью "График"
    {
        bsaFile.data_file.resize(nCounts);
        // устанавливаем количество _единичных_ точек данных nCounts во всём файле, в идеале 36000*336 или 288000*1584
        inClientFile.read((char*) &bsaFile.data_file[0], nCounts * sizeof(float));
        //Считали ВЕСЬ файл!
        mjd_start = Mjd(bsaFile.header.date_begin.mid(6,4).toInt(), bsaFile.header.date_begin.mid(3,2).toInt(), bsaFile.header.date_begin.mid(0,2).toInt(), bsaFile.header.time_begin.mid(0, bsaFile.header.time_begin.length() == 7 ? 1 : 2).toInt(), 0, 0);

        int currentBand = bsaFile.header.nbands;

        QDateTime now = QDateTime::fromString(bsaFile.header.date_begin + (bsaFile.header.time_begin.length() == 7 ? "0" : "") + bsaFile.header.time_begin.mid(0, bsaFile.header.time_begin.length() == 7 ? 1 : 2) + ":00:00", "dd.MM.yyyyhh:mm:ss");
        now.setTimeSpec(Qt::UTC);
        curTime = now.toTime_t(); //получили целочислительное число секунд, прошедшее с начала эпохи UNIX до начала прочтенного файла
                                  //эпоха UNIX началась 01.01.1970 в 00:00:00.
        starTime curStarTime(bsaFile.header.date_begin.mid(6,4).toInt(), bsaFile.header.date_begin.mid(3,2).toInt(),
                             bsaFile.header.date_begin.mid(0,2).toInt(),
                             bsaFile.header.time_begin.mid(0,bsaFile.header.time_begin.length() == 7 ? 1 : 2).toInt(),
                             0.0, 0.0, 2.50875326667, 0);
        //starTime - это класс, вычисляющий звездное время для предложенных параметров (см. qbsa.h)

        now = QDateTime::fromString(QString("01.01.1970") +
              ((int)curStarTime.hour < 10 ? "0" : "") + QString::number((int)curStarTime.hour) + ":" +
              ((int)curStarTime.minute < 10 ? "0" : "") + QString::number((int)curStarTime.minute) + ":" +
              ((int)curStarTime.second < 10 ? "0" : "") + QString::number((int)curStarTime.second, 'f', 0),
               "dd.MM.yyyyhh:mm:ss");
        now.setTimeSpec(Qt::UTC); //получить значение из этой переменной в определённом часовом поясе

        ui->widget->clearGraphs();
        QVector<double> x(bsaFile.header.npoints), y(bsaFile.header.npoints);
        //Создаем вектор длиной во весь файл!

        for (unsigned int currentCanal = 0; currentCanal < bsaFile.header.modulus.size() * bsaFile.nCanal; currentCanal++)
        {//поканальная (получевая) отрисовка данных (Самодуров В.А.) , .modulus.size() - обычно 6, .nCanal=8 , т.е. до 48
            if(ui->comboBox_2->currentIndex() != 0 && (ui->comboBox_2->currentIndex()-1) != currentCanal)
                continue;

            for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
            {
                x[i] = curTime + i * bsaFile.header.tresolution * 0.001;
                y[i] = bsaFile.data_file[nCountPoint * i + currentCanal * (bsaFile.header.nbands+1) + currentBand] + (countsCanal - currentCanal) + 1;
            }          //+ (countsCanal - currentCanal) + 1  - вряд ли это нужно! Плохо потом масштабировать! (Самодуров В.А.)

            ui->widget->addGraph();
            ui->widget->graph(ui->widget->graphCount()-1)->setPen(QPen(Qt::blue));
            ui->widget->graph(ui->widget->graphCount()-1)->setData(x, y);
        }

        ui->widget->xAxis->setRange(curTime, curTime + 0.001 * bsaFile.header.tresolution * bsaFile.header.npoints);
                  //Установили пределы часа в секундах (от начала эпохи UNIX) на графике
        ui->widget->xAxis->setTickLabelType(QCPAxis::ltDateTime);
        ui->widget->xAxis->setDateTimeSpec(Qt::UTC);
        ui->widget->xAxis->setDateTimeFormat("hh:mm:ss");
        ui->widget->xAxis->setLabel("UTC"); //Надо добавить в подпись еще дату!!

        if(ui->comboBox_2->currentIndex() != 0)
            ui->widget->yAxis->setRange(countsCanal - ui->comboBox_2->currentIndex() + 2,
                                        countsCanal - ui->comboBox_2->currentIndex() + 3);
        else
            ui->widget->yAxis->setRange(0.0, bsaFile.header.modulus.size() * bsaFile.nCanal);
              /**  Returns the size() of the largest possible %vector.  */
        ui->widget->xAxis2->setVisible(true);
        ui->widget->xAxis2->setTickLabelType(QCPAxis::ltDateTime);
        ui->widget->xAxis2->setDateTimeSpec(Qt::UTC); // чтобы правильно показывалось звёздное время
        ui->widget->xAxis2->setDateTimeFormat("hh:mm:ss");
        ui->widget->xAxis2->setLabel("RA");

        ui->widget->xAxis2->setRange(now.toTime_t(), now.toTime_t() +
                                     0.001 * bsaFile.header.tresolution * bsaFile.header.npoints);
            //Установили пределы часа в секундах (от начала эпохи UNIX) на графике ВВЕРХУ
        ui->widget->replot();
    }
    else
    {//Отрисовываем файл по кускам длиной spinBox->value() секунд в графические файлы
        unsigned int deriv = ui->spinBox->value(); // количество секунд в порции

        //bsaFile.header.npoints = npoints / 3600 * deriv;
        //  - неверно, ведь концовка файла может отсутствовать, а мы хотели привязаться ко временному отрезку!!!
        bsaFile.header.npoints = (1000/bsaFile.header.tresolution) * deriv;
        //- количество временнЫх точек в порции

        //bsaFile.data_file.resize(nCounts / 3600 * deriv);
        //  - неверно, ведь концовка файла может отсутствовать
        long n_single_points = bsaFile.header.npoints * nCountPoint;
        bsaFile.data_file.resize(n_single_points);
        //Изменили (уменьшили до числа точек внутри порции-кадра) размер массива,
             //перемножили временные на число в одной временной
        // Для целого файла - устанавливали количество _единичных_ точек данных nCounts во всём файле,
        //в идеале 36000*336 или 288000*1584



         //Число точек файла в данном куске из deriv секунд - хранится теперь в изменившемся bsaFile.header.npoints,
         //а вот главное, общее число точек в файле - число npoints - не трогаем!

        int start_point =(int) ( start_time_in_sec / (0.001 * bsaFile.header.tresolution) +0.5);
           //Стартовая точка чтения данных файла, столько надо пропустить!
        // устанавливаем позицию на НУЖНОЕ начало двоичных данных
        //- в соответствии со сдвигом начала чтения файла на start_point временнЫх точек
        int size_point = (bsaFile.header.nbands+1) * bsaFile.nCanal * bsaFile.header.modulus.size();
            //Число данных файла в 1 временной точке, для 6 полос и 48 лучей =336
     //Надо сдвинуться на НУЖНОЕ начало двоичных данных, с которого начнем считывание:
        // ???убрал  inClientFile.seekg(bsaFile.sizeHeader + (start_point*size_point) * sizeof(float), std::ios::beg);
        //- нет, неверно!!! Не учтен номер кадра! Сдвинемся на нужное число байтов уже внутри процедуры учета номера кадров

        stopThread = false;





        otladka=1;
        if (otladka==1)
         {
           /*
            std::string msg;
            std::ofstream fin("D:\\DATA\\LOGS\\output.txt");
            msg = "Our string";
            fin << msg;
            //Получаем системное время и дату
            QDate date = QDate::currentDate();
            QTime time = QTime::currentTime();
            str1 = date.toString("dd.MM.yyyy ")+time.toString("hh:mm:ss.zzz");
            fin << str1.toStdString(); //Запишет в файл системное время
            fin << "\r\n";
            fin.close();
           */

          if(fileOut.open(QIODevice::WriteOnly | QIODevice::Text))
          { // Если файл успешно открыт для записи в текстовом режиме

            writeStream.setCodec(QTextCodec::codecForName("cp1251"));
            writeStream << "Text, text, text. Русский язык.\r\n";
            QDate date = QDate::currentDate();
            QTime time = QTime::currentTime();
            str1 = date.toString("dd.MM.yyyy ")+time.toString("hh:mm:ss.zzz");
            writeStream << str1 ; //Запишет в файл системное время
              // Посылаем строку в поток для записи
            fileOut.flush();
            // fileOut.close(); // Закрываем файл
          };//if(fileOut.open(QIODevice::WriteOnly | QIODevice::Text))
          };//if (otladka==1)


        //unsigned int i=10;
        //start_time_in_sec
        //for (unsigned int i=0; i < 3600 / deriv && !stopThread; i++)
        unsigned int max_i = ui->spinBox_2->value(); // количество порций
        for (unsigned int i=0; i < max_i && !stopThread; i++)
            //i здесь - номер порции для отрисовки (обычно порции по 4 минуты, т.е. до i<15)
        {
            start_point =(int) ( (start_time_in_sec + i*deriv ) / (0.001 * bsaFile.header.tresolution) +0.5);
                       //Стартовая точка чтения данных файла, столько надо пропустить!
            inClientFile.seekg(bsaFile.sizeHeader + (start_point*size_point) * sizeof(float), std::ios::beg);


            /*
            ofstream MyOut_logs ("D:\DATA\LOGS\tmp_1.txt",ios::ate|ios::out);
              //Открыли файл для добавления информации в конец и записи
            MyOut_logs << "BUGAGA" << "\n"; //Записали строчку
            MyOut_logs.close(); //Закрыли открытый файл
            */

            if (otladka==1)
             {
                writeStream << "PORCIA  i,start_point,n_points=" << i << ",  "
                            << start_point << "," << n_single_points << "\n"; //Запишет в файл индекс
                fileOut.flush();
              };//if (otladka==1)




            //inClientFile.read((char*) &bsaFile.data_file[0], nCounts / 3600 * deriv * sizeof(float));
            inClientFile.read((char*) &bsaFile.data_file[0], n_single_points * sizeof(float));

            //??? Что здесь происходит? (Самодуров В.А.)
            //read извлекает блоки (символов) в нужном количестве, заполняя массив bsaFile.data_file
            //(public функция-член std::basic_istream)
            // !!!ВНИМАНИЕ!!! Тут - НЕУЧЕТ номера кадра!!! - индекса i - его тут вообще нет!
            // - ИСПРАВИЛ!!!

            int min_start=i*deriv / 60 + (int) (start_time_in_sec / 60);
            double sec_start=start_time_in_sec + i*deriv - min_start*60;
            /*
            mjd_start = Mjd(bsaFile.header.date_begin.mid(6,4).toInt(),
                         bsaFile.header.date_begin.mid(3,2).toInt(),
                         bsaFile.header.date_begin.mid(0,2).toInt(),
                         bsaFile.header.time_begin.mid(0, bsaFile.header.time_begin.length() == 7 ? 1 : 2).toInt(),
                                                       i*deriv / 60, 0.0);
                                                       */
            mjd_start = Mjd(bsaFile.header.date_begin.mid(6,4).toInt(),
                         bsaFile.header.date_begin.mid(3,2).toInt(),
                         bsaFile.header.date_begin.mid(0,2).toInt(),
                         bsaFile.header.time_begin.mid(0, bsaFile.header.time_begin.length() == 7 ? 1 : 2).toInt(),
                                                       min_start, sec_start);
            //посчитали Mjd старта отрисовки из UTC


            //Считаем как starTime(int Year = 2000, int Month = 1, int Date = 1, int hr = 0,
            //int min = 0, double sec = 0, double longitude_in_h = 2.50875326667, int d_h_from_UT = -4)
            /*
            starTime curStarTime(bsaFile.header.date_begin.mid(6,4).toInt(),
                         bsaFile.header.date_begin.mid(3,2).toInt(),
                         bsaFile.header.date_begin.mid(0,2).toInt(),
                         bsaFile.header.time_begin.mid(0, bsaFile.header.time_begin.length() == 7 ? 1 : 2).toInt(),
                                                       i*deriv / 60, 0.0, 2.50875326667, 0);
                                                       */
            starTime curStarTime(bsaFile.header.date_begin.mid(6,4).toInt(),
                         bsaFile.header.date_begin.mid(3,2).toInt(),
                         bsaFile.header.date_begin.mid(0,2).toInt(),
                         bsaFile.header.time_begin.mid(0, bsaFile.header.time_begin.length() == 7 ? 1 : 2).toInt(),
                                                       min_start, sec_start, 2.50875326667, 0);
            //посчитали звездное время старта отрисовки из UTC и долготы Пущино
            pStarTime = &curStarTime;// - и захватили его в указатель


            //QString tewmp = QString::fromStdString(bsaFile.header.date_begin + static_cast<std::string> (bsaFile.header.time_begin.length() == 7 ? "0" : "") + bsaFile.header.time_begin.substr(0, bsaFile.header.time_begin.length() == 7 ? 1 : 2) + ":" + ((i*deriv / 60) < 10 ? "0" : "")) + QString::number(i*deriv / 60, 10) + ":00";
            /*
            QDateTime now = QDateTime::fromString(bsaFile.header.date_begin
               + (bsaFile.header.time_begin.length() == 7 ? "0" : "")
               + bsaFile.header.time_begin.mid(0, bsaFile.header.time_begin.length() == 7 ? 1 : 2)
               + ":" + ((i*deriv / 60) < 10 ? "0" : "")
               + QString::number(i*deriv / 60, 10) + ":00", "dd.MM.yyyyhh:mm:ss");
            now.setTimeSpec(Qt::UTC);
            */

            QString date_and_time=bsaFile.header.date_begin + " " + bsaFile.header.time_begin;
            QDateTime now = QDateTime::fromString(date_and_time, "dd.MM.yyyy HH:mm:ss");
            now = now.addSecs(i*deriv);
            //Прибавили число секунд от старта файла для перехода на начало порции
            now = now.addMSecs((int)(start_time_in_sec*1000));
            //Прибавили число секунд от старта файла
            now.setTimeSpec(Qt::UTC);
            //Определили UTC старта отрисовки

            curTime = now.toTime_t();
            //Текущее время - взяли как UTC,превратили в  curTime  - это UNIX секунды

            //При входе в runSaveFile у нас есть глобальные переменные curTime , bsaFile.header.npoints
            total = 0.0;
            if (otladka==1)
             {
                //str1 = now.toString;

                writeStream << "\n DO runSaveFile total, " << total << "  time=" ; //Запишет в файл параметры
                str1 = now.toString("yyyy.MM.dd hh:mm:ss.zzz");
                writeStream.setRealNumberNotation(QTextStream::FixedNotation);
                //ИЛИ: writeStream.setRealNumberNotation(QTextStream::ScientificNotation);
                //writeStream << str1 << "=in sec=" << qSetFieldWidth(12) << left << curTime << "\n\n"; //Запишет в файл системное время
                writeStream << str1 << "=in sec=" << curTime << "\n\n"; //Запишет в файл системное время
                fileOut.flush();
              };//if (otladka==1)
            runSaveFile();
            // !!!Вот здесь все сделали - считали, откалибровали, нарисовали 1 временнУю порцию файлов!!!

            progress_ptr->setValue(100.0 * (i+1) * deriv / 3600);

            qApp->processEvents();// обработка ожидающих в очереди событий
        }; //for (unsigned int i=0; i < max_i && !stopThread; i++)


        if (otladka==1)
         {
            fileOut.flush();
            fileOut.close(); // Закрываем файл
          };//if (otladka==1)


        //nnn

    }

    QDateTime finish = QDateTime::currentDateTime();
    if(!ui->checkBox_2->isChecked())
        QMessageBox::information(this, "Сообщение", "Время " + QString::number(finish.secsTo(start)) + " сек ");

    inClientFile.close();

    progress_ptr->hide();
}

void grav::runSaveFile()//Главная функция для отрисовки файла
{
    float add_tmp, y_first,  y_second, add_channel_in_graph, y_metka; //переменные для тестов
    float array_1_mem[10], array_2_mem[10];
           //массивы в память для тестов - туда кладем с векторов что-либо для вывода в режиме отладки
    add_channel_in_graph=0;// > 0 - сдвиг на определенное число каналов вверх всего рисунка
    unsigned int currentBand;
    unsigned int i;
    double calib_data_T; //Переменная для хранения текущих откалиброванных данных (в Кельвинах)
    int otladka_add_1=0; //Для добавочного отладочного коэффициента, который будет употреблен в калибровочной функции
    double temp; //Для запоминания температуры (после калибровки) предпоследней точки последнего (нижнего) луча

    QString OutLine_1; //для формирования меток на оси

    //PlotSettings settings;
    //График рисуем


    int size_point = bsaFile.data_file.size() / bsaFile.header.npoints;
      //Число данных в 1 временной точке, для 6 полос и 48 лучей =336
    double start_time_in_sec = ui->doubleSpinBox->value();
       // стартовая секунда, с которой надо отрисовывать файлы порций;
    // int start_point =(int) ( start_time_in_sec / (0.001 * bsaFile.header.tresolution) +0.5);
       //Стартовая точка
    double multiply_y = ui->doubleSpinBox_2->value();
       //масштабный коэффициент графика по Y (чем больше, тем крупнее масштаб)
    double scale_T = ui->doubleSpinBox_3->value();
      //масштабный отрезок температуры, который пририсовывается справа внизу по правой оси Y (и на данных впритык к ней)
    double add_multiply=ui->doubleSpinBox_4->value();
     // add_multiply - сжимающий графики по оси Y коэффициент - втискивает графики из многих строк в окно
    QDateTime now = QDateTime::fromString(QString("01.01.1970") + ((int)pStarTime->hour < 10 ? "0" : "")
                                          + QString::number((int)pStarTime->hour) + ":"
                                          + ((int)pStarTime->minute < 10 ? "0" : "")
                                          + QString::number((int)pStarTime->minute) + ":"
                                          + ((int)pStarTime->second < 10 ? "0" : "")
                                          + QString::number((int)pStarTime->second, 'f', 0), "dd.MM.yyyyhh:mm:ss");
    now.setTimeSpec(Qt::UTC);
    //Выставили время на начало эпохи UNIX

    //double koeff = (bsaFile.name_file_of_datas.indexOf("_n2_", 0, Qt::CaseInsensitive) != -1) ? 1600.0 : 400.0;
    //- у Торопова. Вылетало, если
    double koeff = (bsaFile.name_file_of_datas.indexOf("_n2_", 0, Qt::CaseInsensitive) != -1) ? 400.0 : 400.0;
    //(Условие) ? (выражение1):(выражение2)
    // Если условие истинно, то выполняется выражение1, а если ложно, то выражение2.
    //???koeff - поправляет зону рисования на оси Y, если это файл второй стойки???  БЫЛО: 400

    //double koeff = 400.0;

    unsigned int countsCanal = bsaFile.header.modulus.size() * bsaFile.nCanal;
     // количество каналов - точнее, ЛУЧЕЙ БСА, покрываемых файлом. Обычно равно 48
    double tresolution = 0.001 * bsaFile.header.tresolution;
     //реальное временнОе разрешение по 1 точке - в секундах
    QVector< QVector<QPointF>> curve_vecs0, curve_vecs1, curve_vecs2;
      //Класс QPointF описывает точку на плоскости, используя точность плавающей запятой.
      //                                    http://doc.crossplatform.ru/qt/4.5.0/qpointf.html
      //Точка определяется координатой x и координатой y, которые могут быть доступны используя функции x() и y().
      //Координаты точки определяются используя точность чисел с плавающей запятой.

    QMap<double, QString> listY; // список меток по оси Y (слева)

    QMap<double, QString> listY_2; // список меток по оси Y (справа)
    float array_metka_Y2[countsCanal];// МАССИВ меток по оси Y (справа) - по-лучам

    std::string tmp_string; //(Самодуров В.А.)
    // tmp_string=bsaFile.header.date_begin.toStdString().c_str() + "UTC";
    std::string tmp_str_1="UTC  ";
    tmp_string=tmp_str_1 + bsaFile.header.date_begin.toStdString().c_str();
    /*
       Функция c_str() возвращает указатель на символьный массив,
       содержащий строку объекта string в том виде,
       в каком она находилась бы во встроенном строковом типе.
    */
    const char *str_X = tmp_string.c_str(); // правильно, теперь в ней содержится нижняя подпись


    if (otladka==1)
     {
        //str1 = now.toString;

        writeStream << "\n IN runSaveFile, pered prohodom bsaFile.header.npoints,total,koeff,curTime= "
                    << bsaFile.header.npoints <<  ","  << total << "," << koeff << "," << curTime << "\n"; //Запишет в файл параметры
        fileOut.flush();
      };//if (otladka==1)

    if(ui->comboBox_2->currentIndex() == 0)
    {
        //работаем по полосам, для каждой полосы частот создаем свою картинку
        unsigned int startBand = 0;
        unsigned int finishBand = bsaFile.header.nbands;

        if(ui->comboBox->currentIndex() != 0)
            startBand = finishBand = ui->comboBox->currentIndex()-1;
        else if(ui->checkBox->isChecked()) //Чек-бокс "Сдвиг"
        {
            curve_vecs0.reserve(countsCanal * bsaFile.header.npoints);
            curve_vecs1.reserve(countsCanal * bsaFile.header.npoints * bsaFile.header.nbands);
        }

        for (currentBand = startBand; currentBand <= finishBand; currentBand++)
         {//Одну за другой создаем 48-ми лучевые картинки ДЛЯ КАЖДОЙ ПОЛОСЫ
            widget_2.clearCurves();

            for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)
                //Проход по лучам БСА
              {
                QVector<QPointF> curve_vec(bsaFile.header.npoints);
                total = 0.0;

                for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
                   //Проход по временнЫм точкам
                  {
                    //!!!!!!!!
                    /*
                    curve_vec[i] = QPointF(tresolution *i + curTime,
                                       //это - Х, отталкиваемся от текущего времени, добавляя i точек
                          //calib(double y, double ss, int numberBand, int numberCanal)
                                     calib(bsaFile.data_file[size_point * i //(i+start_point)
                                             + currentCanal * (bsaFile.header.nbands+1)
                                             + currentBand],//
                           tresolution * i, currentBand, currentCanal) //первое - секунды от начала файла
                            // / koeff + (countsCanal - currentCanal));
                             /koeff );
                      //Это - Y, калибровка данных в каждой точке i - по времени,
                      //currentCanal - номер луча, currentBand - номер полосы
                      //При этом внутри calib накапливается суммой total += y_Kelvin;
                      */

                    //if ( ((otladka==1) && (currentCanal==countsCanal-1) && (currentBand==startBand))
                    //     && ((i==0) || (i==bsaFile.header.npoints-1)) )
                    if ( ((otladka==1) && (currentCanal==countsCanal-1))
                             && ( i==bsaFile.header.npoints-1 ) )
                      {
                        otladka_add_1=1;
                        int tmp_i=size_point * i
                                + currentCanal * (bsaFile.header.nbands+1)
                                + currentBand;
                        writeStream << "PERED CALB: size_point,i,Canal,nbands,currentBand , tmp_i, y= "
                                    << size_point << "," << i << "," << currentCanal << "," << bsaFile.header.nbands
                                    << "," << currentBand << "," << tmp_i
                                    << "," << bsaFile.data_file[tmp_i]
                                    << "\n"; //Запишет в файл параметры

                      }//if (otladka==1)
                    else
                      {
                       otladka_add_1=0;
                      };

                    calib_data_T=calib(bsaFile.data_file[size_point * i //(i+start_point)
                            + currentCanal * (bsaFile.header.nbands+1)
                            + currentBand],//
                            tresolution * i, currentBand, currentCanal, otladka_add_1);
                    //Откалибровали к градусам Кельвина значение данных
                    otladka_add_1=0;
                    //Сбросили на всякий случай значение вывода отладочных значений внутри калибровки к НЕ-выводу
                    if((i==bsaFile.header.npoints-1) && (currentCanal==countsCanal-1 ))
                     //Захватываем в значение метки на правой оси графика значение последней точки последнего (нижнего) канала
                     {
                       temp=calib_data_T;
                     };

                    curve_vec[i] = QPointF(tresolution *i + curTime,  calib_data_T / koeff);
                    //Приняли значение вектора на графике (поделив на масштабный коэффициент

                    if ( ((otladka==1) && (currentCanal==countsCanal-1) && (currentBand==startBand))
                         && ((i==0) || (i==bsaFile.header.npoints-1)) )
                     {
                        writeStream << "band,ray,i,calb_T= "
                                    << currentBand << ", " << currentCanal << ", " << i << ", "
                                    << calib_data_T << "\n"; //Запишет в файл параметры
                      };//if (otladka==1)
                  };
                  //Проход по временнЫм точкам for (unsigned int i=0; i < bsaFile.header.npoints; ++i)

                double y_average = total / (koeff * bsaFile.header.npoints);

                for (i=0; i < bsaFile.header.npoints; ++i)
                   {
                    //curve_vec[i].setY( (curve_vec[i].y() - y_average)*multiply_y + (countsCanal - currentCanal));
                    //Стандартный вариант, без отладки

                    if((i==bsaFile.header.npoints-2) && (currentCanal==countsCanal-1 ))
                     {
                       add_tmp=scale_T/koeff ;   /* 100.0/koeff */
                       y_first=curve_vec[i].y();
                       curve_vec[i].setY( (curve_vec[i].y() + add_tmp - y_average)*multiply_y
                                         + (countsCanal+add_channel_in_graph - currentCanal) * add_multiply);
                       y_second=curve_vec[i].y();
                       array_1_mem[0]=y_first;
                       array_1_mem[1]=(y_first - y_average)*multiply_y +
                               (countsCanal+add_channel_in_graph - currentCanal)  * add_multiply;
                       array_1_mem[2]=(y_first + add_tmp - y_average)*multiply_y +
                               (countsCanal+add_channel_in_graph - currentCanal) * add_multiply;

                       y_metka=curve_vec[i].y();
                       OutLine_1="+" + OutLine_1.setNum(scale_T) + " (10~1Jy)";

                       // !!! settings.mListY2.insert(y_metka,OutLine_1);

                       listY_2.insert(y_metka, OutLine_1);
                       //Метки на оси Y справа
                       //Вставили верхнюю метку (соответствует сдвинутому вверх значению)
                     }
                    //Искуственно добавляем 100 К (нет, теперь scale_T !) чтобы вывести реальный масштаб!
                    else
                     {
                       curve_vec[i].setY( (curve_vec[i].y() - y_average)*multiply_y +
                                          (countsCanal+add_channel_in_graph - currentCanal)* add_multiply);
                     };//Назначили координату Y данным в точке i

                   }; //(unsigned int i=0; i < bsaFile.header.npoints; ++i)

                // !!!!!!!!!!!!!!!!!!!!!!!
                if ((otladka==1) && (currentCanal==countsCanal-1) && (currentBand==startBand))
                 {
                    //str1 = now.toString;
                    //float tmp1=curve_vec[0];
                    writeStream << "IN runSaveFile, currentCanal, i, curve_vec[0],curve_vec[bsaFile.header.npoints-1],npoints= "
                                << currentCanal << "," << i << ","  << curve_vec[0].y() << "," << curve_vec[bsaFile.header.npoints-1].y()
                               << "," << bsaFile.header.npoints << "\n"; //Запишет в файл параметры
                    fileOut.flush();
                  };//if (otladka==1)

                //y_second=y_second; // Отладочная строка
                widget_2.addCurve(curve_vec,
                                  QColor(255 - 255 / bsaFile.header.nbands * currentBand,
                                         255 / (4*bsaFile.header.nbands) * currentBand,
                                         255 / bsaFile.header.nbands * currentBand)); // Qt::blue);

                listY.insert(curve_vec[0].y(), ui->comboBox_2->itemText(currentCanal+1));
                //Метки на оси Y слева ()1- число - Y, второе - строка формата :
                //взяли Y от 0-го элемента curve_vec, а текст метки сгененировали при помощи itemText(currentCanal+1)

               /* widget_2.addCurve(curve_vec,
                               QColor(255 - 255 / bsaFile.header.nbands * currentBand - (countsCanal - currentCanal)/2,
                                      255 / (4*bsaFile.header.nbands) * currentBand + (countsCanal - currentCanal)/2,
                                      255 / bsaFile.header.nbands * currentBand )); // Qt::blue); */


                //формируем правую метку на последней точке канала
                array_metka_Y2[currentCanal] = curve_vec[bsaFile.header.npoints-1].y();

                if ((otladka==1) && (currentCanal==countsCanal-1) && (currentBand==startBand))
                 {
                    //str1 = now.toString;
                    //float tmp1=curve_vec[0];
                    writeStream << "IN METKA , currentBand,last-1,last= "
                                << currentBand << "," << curve_vec[bsaFile.header.npoints-2].y() << "," << curve_vec[bsaFile.header.npoints-1].y()
                                << "\n"; //Запишет в файл параметры
                    fileOut.flush();
                  };//if (otladka==1)

              };
              //Проход по лучам БСА for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)
            PlotSettings settings;
            //settings.setAxis("X", curTime, curTime + tresolution * bsaFile.header.npoints, "UTC");
             //выставляем метки по времени UTC снизу

            //tmp_string=bsaFile.header.date_begin.toStdString() + "UTC";
            //tmp_string="2017-05-30 " + "UTC";
            //settings.setAxis("X", curTime, curTime + tresolution * bsaFile.header.npoints,tmp_string);
            settings.setAxis("X", curTime, curTime + tresolution * bsaFile.header.npoints,str_X);

            settings.setAxis("Y", 0.0, countsCanal);
            settings.setAxis("X2", now.toTime_t(),
                             now.toTime_t() + tresolution * bsaFile.header.npoints * 86164.090530833/86400.0,
                             "RA", true, Qt::darkGreen);
             //выставляем метки по времени RA сверху
            settings.setAxis("Y2", 0.0, countsCanal, "", true);


            array_2_mem[0]=total;
            if ((otladka==1) && (otladka_add_1==1))
             {
                //str1 = now.toString;
                //float tmp1=curve_vec[0];
                writeStream << "PERED CALB TEMP: countsCanal, nbands,finishBand , y= "
                            << currentBand << "," << bsaFile.header.nbands << "," << finishBand
                            << "," << bsaFile.data_file[(countsCanal-1) * (bsaFile.header.nbands+1) + finishBand]
                            << "\n"; //Запишет в файл параметры
                fileOut.flush();
              };//if (otladka==1)

           /* Внимание! Правильный temp захвачен выше, а перевычисление - на самом деле было неверным, закомментарили.
            temp = calib(bsaFile.data_file[(countsCanal-1) * (bsaFile.header.nbands+1) + finishBand],
                    0.0, currentBand, countsCanal-1,otladka_add_1);
           */
            array_2_mem[1]=temp;
            total=array_2_mem[0];
            //Восстанавливаем искаженное лишней калибровкой значение total
           /*
             БЫЛО в самом НАЧАЛЕ:
            settings.mListY2.insert(1000.0 * ( listY.firstKey() - 1 ) /
( calib(bsaFile.data_file[(countsCanal-1) * (bsaFile.header.nbands+1) + finishBand],
0.0, finishBand, countsCanal-1) - total / bsaFile.header.npoints ), "1000K ~ 100Jy");
            */



            //Возьмем крайнее правое значение по Y последнего (нижнего) луча - как начальную метку 0 ,
            //но с правильным значением - для правой оси
            //curve_vec[bsaFile.header.npoints-1].y();
            //OutLine_1="i=" + QString::number(i) + " K";
            OutLine_1="T=" + OutLine_1.setNum(temp) + " K";

            y_metka=array_metka_Y2[countsCanal-1];

            listY_2.insert(y_metka, OutLine_1);
            //Метки на оси Y справа
            //Вставили нижнюю метку (соответствует значению последней точки)


            //   !!!!    settings.mListY2.insert(y_metka,OutLine_1);
            //Вставили нижнюю метку (соответствует значению последней точки)

            /*
            y_metka=y_metka+(y_second-y_first)+1/(add_multiply*add_multiply);
            OutLine_1="+" + OutLine_1.setNum(scale_T) + " (10~1Jy)";
            settings.mListY2.insert(y_metka,OutLine_1);
            */  //Пока убираю , вводя выше вторую метку





            settings.autoAdjust = false;
            settings.mTickLabelTypeX = PlotSettings::mtDateTime;
            settings.mDateTimeFormatX = "hh:mm:ss";

#ifdef Q_OS_WIN
        settings.MarginX = 80;
#endif

#ifdef Q_OS_LINUX
        settings.MarginX = 100;
#endif

            settings.autoAdjustY = false;
            settings.mTickLabelTypeY = PlotSettings::mtList;
            settings.mListY.swap(listY);
            settings.numYTicks = settings.mListY.count() + 1;

            settings.autoAdjustY2 = false;
            //!!! -
            settings.mListY2.swap(listY_2);

            settings.numY2Ticks = 2;//1

            if(currentBand == bsaFile.header.fbands.size())
                settings.title = "Частота усреднённая (109.0-111.5 МГц)";
            else
                settings.title = "Частота " + QString::number(bsaFile.header.fbands[currentBand],'f', 3) + " МГц";

            widget_2.setPlotSettings(settings);

            if(!ui->widget->isHidden())
                widget_2.update();

            // сохранение в файл
            if(!ui->checkBox->isChecked())
                widget_2.save(ui->leDir->text() + QString::number(mjd_start, 'f', 5)
                              + "_" + QString::number(currentBand+1) + "_band.png", "PNG", 1200, 600);

            // сохранение со сдвигом
            if(ui->comboBox->currentIndex() == 0 && ui->checkBox->isChecked())
            {
                if(currentBand == 0)
                    curve_vecs0.swap(widget_2.curve_vecs); // 0-я частота
                else
                    foreach (QVector<QPointF> curve_vec, widget_2.curve_vecs)
                    {
                        int index = widget_2.curve_vecs.indexOf(curve_vec);
                        for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
                            curve_vec[i].setY(curve_vec[i].y() - curve_vecs0[index][i].y() + (countsCanal - index));

                        curve_vecs1.append(curve_vec);
                    }

                if(currentBand == finishBand)
                {
                    settings.title = "Сдвиг по каналам";
                    widget_2.setPlotSettings(settings);
                }
            };
            // сохранение со сдвигом, конец: if(ui->comboBox->currentIndex() == 0 && ui->checkBox->isChecked())

         };
        //for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)
        //Закончили создание 48-ми лучевых картиноки ДЛЯ КАЖДОЙ ПОЛОСЫ

        // !!!!!!!!!!!!!!!!!!!!!!!
        if (otladka==1)
         {
            //str1 = now.toString;

            writeStream << "\n IN runSaveFile, posle polos  startBand,currentBand,total= "
                        << startBand <<  ","  << currentBand << "," << total << "\n"; //Запишет в файл параметры
            fileOut.flush();
          };//if (otladka==1)


        // сохранение со сдвигом
        if(!curve_vecs1.empty())
        {
            widget_2.clearCurves();
            foreach (QVector<QPointF> curve_vec, curve_vecs1)
                widget_2.addCurve(curve_vec, QColor(255 - 255 / bsaFile.header.nbands * curve_vecs1.indexOf(curve_vec) / countsCanal,
                                                    255 / (4*bsaFile.header.nbands) * curve_vecs1.indexOf(curve_vec) / countsCanal,
                                                    255 / bsaFile.header.nbands * curve_vecs1.indexOf(curve_vec) / countsCanal));

            widget_2.save(ui->leDir->text() + QString::number(mjd_start, 'f', 5) + "_bands1.png", "PNG", 1200, 600);
        }

    }
    else // для 1 луча создаем картинку - проход по частотам снизу вверх, 33 частоты
    {
        // по лучам (для луча создаем картинку - проход по частотам снизу вверх, 33 частоты)
        unsigned int currentCanal = ui->comboBox_2->currentIndex()-1;
         //номер канала нашли

        if(ui->checkBox->isChecked())
        {
            curve_vecs0.reserve(bsaFile.header.npoints);
            curve_vecs1.reserve(bsaFile.header.npoints * bsaFile.header.nbands);
        }

        widget_2.clearCurves();
        for (unsigned int currentBand = 0; currentBand <= bsaFile.header.nbands; currentBand++)
        {
            QVector<QPointF> curve_vec(bsaFile.header.npoints);
            total = 0.0;

            for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
            {
                /* !!!!!!!!!!! Было ранее.
                curve_vec[i] = QPointF(tresolution * i + curTime,
                    calib(bsaFile.data_file[size_point * i + currentCanal * (bsaFile.header.nbands+1) + currentBand],
                        tresolution * i, currentBand, currentCanal,otladka_add_1) / koeff + currentBand + 1);
                        */
                curve_vec[i] = QPointF(tresolution * i + curTime,
                                       calib(bsaFile.data_file[size_point * i + currentCanal * (bsaFile.header.nbands+1) + currentBand],
                                           tresolution * i, currentBand, currentCanal,otladka_add_1)
                                      / koeff + (currentBand + 1) * add_multiply);
            };

            double y_average = total / (koeff * bsaFile.header.npoints);

            /* for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
                curve_vec[i].setY(curve_vec[i].y() - total / (koeff * bsaFile.header.npoints));*/
            for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
                curve_vec[i].setY((curve_vec[i].y() - y_average)*multiply_y);

            /* Для (48) лучей было:
              curve_vec[i].setY( (curve_vec[i].y() - y_average)*multiply_y +
                                  (countsCanal+add_channel_in_graph - currentCanal)* add_multiply);
               //Назначили координату Y данным в точке i
             */

            /* curve_vec[i].setY( (curve_vec[i].y() - y_average)*multiply_y +
                               (countsCanal+add_channel_in_graph - currentCanal)* add_multiply);*/

            widget_2.addCurve(curve_vec, QColor(255 - 255 / bsaFile.header.nbands * currentBand,
                                                255 / (4*bsaFile.header.nbands) * currentBand,
                                                255 / bsaFile.header.nbands * currentBand)); // Qt::blue);

            listY.insert(curve_vec[0].y(), QString::number(currentBand+1) + " ("
                    + QString::number(currentBand == bsaFile.header.nbands ? (*bsaFile.header.fbands.begin()
                    + *bsaFile.header.fbands.rbegin())/2 : bsaFile.header.fbands[currentBand], 'g', 6) + "М)");
            //Подписали слева на оси номера частот и частоту в мегагерцах

            if (currentBand == 0)//Вставляем метки значения справа от нижнего частотного канала
               {
                /*
                 * curve_vec[i] = QPointF(tresolution * i + curTime,
                                       calib(bsaFile.data_file[size_point * i + currentCanal * (bsaFile.header.nbands+1) + currentBand],
                                           tresolution * i, currentBand, currentCanal,otladka_add_1)
                                      / koeff + (currentBand + 1) * add_multiply);
                 */
                //y_metka=curve_vec[bsaFile.header.npoints-1].y();
                temp=calib(bsaFile.data_file[size_point * (bsaFile.header.npoints-1)
                        + currentCanal * (bsaFile.header.nbands+1)
                        + currentBand],
                        tresolution * (bsaFile.header.npoints-1),
                        currentBand, currentCanal,otladka_add_1);
                y_metka=((temp / koeff + (currentBand + 1) * add_multiply) - y_average)*multiply_y ;
                OutLine_1="T=" + OutLine_1.setNum(temp) + " K";
                listY_2.insert(y_metka, OutLine_1);
                                //Метки на оси Y справа
                                //Вставили нижнюю метку (соответствует значению последней точки)


                //y_metka=(temp+scale_T) / koeff + (currentBand + 1) * add_multiply;
                y_metka=(((temp+scale_T) / koeff + (currentBand + 1) * add_multiply) - y_average)*multiply_y ;
                OutLine_1="+" + OutLine_1.setNum(scale_T) + " (10~1Jy)";
                listY_2.insert(y_metka, OutLine_1);
                                //Метки на оси Y справа
                                //Вставили верхнюю метку (соответствует сдвинутому вверх значению)


               };// if (currentBand == 0)//Вставляем метку значения справа от нижнего частотного канала

            if(ui->checkBox->isChecked())
                if(currentBand == 0)
                    curve_vecs0.append(curve_vec);
                else
                {
                    curve_vecs1.append(curve_vec);
                    for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
                        /* curve_vecs1.last()[i].setY(curve_vecs1.last().at(i).y()
                            - curve_vecs0[0][i].y() - currentBand + (countsCanal - currentCanal));*/
                        curve_vecs1.last()[i].setY(curve_vecs1.last().at(i).y()
                             - curve_vecs0[0][i].y() - (currentBand - (countsCanal - currentCanal))* add_multiply);

                    /* Для (48) лучей было:
                      curve_vec[i].setY( (curve_vec[i].y() - y_average)*multiply_y +
                                          (countsCanal+add_channel_in_graph - currentCanal)* add_multiply);
                       //Назначили координату Y данным в точке i
                     */

                }
        }
          //for (unsigned int currentBand = 0; currentBand <= bsaFile.header.nbands; currentBand++)

        PlotSettings settings;
        //settings.setAxis("X", curTime, curTime + tresolution * bsaFile.header.npoints, "UTC");
        //Подписываем ось Х внизу.
        settings.setAxis("X", curTime, curTime + tresolution * bsaFile.header.npoints,str_X);

        settings.setAxis("Y", 0.0, bsaFile.header.nbands + 2);
        settings.setAxis("X2", now.toTime_t(), now.toTime_t() + tresolution * bsaFile.header.npoints * 86164.090530833/86400.0,
                         "RA", true, Qt::darkGreen);
        settings.setAxis("Y2", 0.0, bsaFile.header.nbands + 2, "", true);

        settings.autoAdjust = false;




        /* Было:
        settings.mListY2.insert(1000.0 * ( listY.lastKey() - (bsaFile.header.nbands+1) )
                        / ( calib(bsaFile.data_file[currentCanal * (bsaFile.header.nbands+1) + bsaFile.header.nbands],
                          0.0, bsaFile.header.nbands, currentCanal,otladka_add_1) - total / bsaFile.header.npoints ), "1000K ~ 100Jy");
                          */


        settings.autoAdjustY = false;
        settings.mTickLabelTypeY = PlotSettings::mtList;
        settings.mListY.swap(listY);
        settings.numYTicks = bsaFile.header.nbands + 1;

        settings.autoAdjustY2 = false;
        //!!! -
        settings.mListY2.swap(listY_2);
        settings.numY2Ticks = 2;//1

#ifdef Q_OS_WIN
        settings.MarginX = 80;
#endif

#ifdef Q_OS_LINUX
        settings.MarginX = 100;
#endif

        settings.mTickLabelTypeX = PlotSettings::mtDateTime;
        settings.mDateTimeFormatX = "hh:mm:ss";
        settings.title = "Луч " + QString::number(currentCanal+1) + ". Частоты с " + QString::number(*bsaFile.header.fbands.begin(),'f', 3) + " по " + QString::number(*bsaFile.header.fbands.rbegin(),'f', 3) + " МГц";

        widget_2.setPlotSettings(settings);

        if(!ui->widget->isHidden())
            widget_2.update();

        // сохранение в файл
        if(!ui->checkBox->isChecked())
            widget_2.save(ui->leDir->text() + QString::number(mjd_start, 'f', 5) + "_" + QString::number(currentCanal+1) + "_canal.png", "PNG", 1200, 600);

        // сохранение со сдвигом
        if(!curve_vecs1.empty())
        {
            widget_2.clearCurves();
            foreach (QVector<QPointF> curve_vec, curve_vecs1)
                widget_2.addCurve(curve_vec, QColor(255 - 255 / bsaFile.header.nbands * curve_vecs1.indexOf(curve_vec), 255 / (4*bsaFile.header.nbands) * curve_vecs1.indexOf(curve_vec), 255 / bsaFile.header.nbands * curve_vecs1.indexOf(curve_vec)));

            settings.title = "Сдвиг по каналу " + QString::number(currentCanal+1);
            settings.setAxis("Y", 0.0, bsaFile.header.modulus.size() * bsaFile.nCanal);
            widget_2.setPlotSettings(settings);

            widget_2.save(ui->leDir->text() + QString::number(mjd_start, 'f', 5) + "_canal1_" + QString::number(currentCanal+1) + ".png", "PNG", 1200, 600);
        }
    }
     //else: По частотам одного луча
}

void grav::on_progress_cancel()
{
    stopThread = true;
}


void grav::on_pushButton_3_toggled(bool checked)
{
    if(checked)
        layout()->setSizeConstraint(QLayout::SetMaximumSize);
    else
        layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void grav::mousePress(QMouseEvent *event)
{
  // if an axis is selected, only allow the direction of that axis to be dragged
  // if no axis is selected, both directions may be dragged

  QCustomPlot *pActive = (QCustomPlot*)sender();

  if (pActive->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    pActive->axisRect()->setRangeDrag(pActive->xAxis->orientation());
  else if (pActive->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    pActive->axisRect()->setRangeDrag(pActive->yAxis->orientation());
  else
    pActive->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);

}

void grav::mouseWheel(QWheelEvent *event)
{
  // if an axis is selected, only allow the direction of that axis to be zoomed
  // if no axis is selected, both directions may be zoomed

  QCustomPlot *pActive = (QCustomPlot*)sender();

  if (pActive->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    pActive->axisRect()->setRangeZoom(pActive->xAxis->orientation());
  else if (pActive->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    pActive->axisRect()->setRangeZoom(pActive->yAxis->orientation());
  else
    pActive->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
}

void grav::selectionChanged()
{
  /*
   normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
   the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
   and the axis base line together. However, the axis label shall be selectable individually.

   The selection state of the left and right axes shall be synchronized as well as the state of the
   bottom and top axes.

   Further, we want to synchronize the selection of the graphs with the selection state of the respective
   legend item belonging to that graph. So the user can select a graph by either clicking on the graph itself
   or on its legend item.
  */

  QCustomPlot *pActive = (QCustomPlot*)sender();

  // make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
  if (pActive->xAxis->selectedParts().testFlag(QCPAxis::spAxis) || pActive->xAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
      pActive->xAxis2->selectedParts().testFlag(QCPAxis::spAxis) || pActive->xAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    pActive->xAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    pActive->xAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }
  // make left and right axes be selected synchronously, and handle axis and tick labels as one selectable object:
  if (pActive->yAxis->selectedParts().testFlag(QCPAxis::spAxis) || pActive->yAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
      pActive->yAxis2->selectedParts().testFlag(QCPAxis::spAxis) || pActive->yAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    pActive->yAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    pActive->yAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }

}

void grav::setRange(QCPRange range)
{
    QDateTime time_lower;
    time_lower.setTime_t(range.lower);

    QDateTime time_upper;
    time_upper.setTime_t(range.upper);

    QCPRange range2;

    starTime starTimeLower(time_lower.toString("yyyy").toInt(),time_lower.toString("M").toInt(),time_lower.toString("d").toInt(),time_lower.toString("h").toInt(),time_lower.toString("m").toInt(),time_lower.toString("s").toDouble(), 2.50875326667, -3);
    range2.lower = QDateTime(QDate(1970,1,1), QTime(starTimeLower.hour, starTimeLower.minute, floor(starTimeLower.second)), Qt::UTC).toTime_t();

    starTime starTimeUpper(time_upper.toString("yyyy").toInt(),time_upper.toString("M").toInt(),time_upper.toString("d").toInt(),time_upper.toString("h").toInt(),time_upper.toString("m").toInt(),time_upper.toString("s").toDouble(), 2.50875326667, -3);
    range2.upper = QDateTime(QDate(1970,1,(starTimeUpper.hour < starTimeLower.hour ? 2 : 1)), QTime(starTimeUpper.hour, starTimeUpper.minute, floor(starTimeUpper.second)), Qt::UTC).toTime_t();

    ui->widget->xAxis2->setRange(range2);
}

void grav::on_toolButton_3_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите директорию сохранения файлов"), "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty())
        return;

    ui->leDir->setText(dir);
}

void grav::on_pbSelectHR_clicked()
{
    chooseFile();
}

void grav::on_tbHelp_clicked()
{

}


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

//разделить строку на элементы (подстроки) указанным разделителем
std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

//Читаем параметры работы (обработки данных) из текстового файла инициализации параметров и выдаем их наружу
void read_from_file_ini_for_find_of_Dispers(QString name_file_of_ini,
                                            int &do_dispers,
                                            float &limit_dispersion_min,
                                            float &limit_dispersion_max,
                                            float &limit_myltiply,
                                            float &limit_sigma,
                                            float &multiply_y,
                                            float &scale_T,
                                            float &add_multiply,
                                            unsigned int &deriv,
                                            std::string &out_data_dir,
                                            std::string &dispers_data_file,
                                            float &limit_Alpha_min,
                                            float &limit_Alpha_max,
                                            float &limit_Delta_min,
                                            float &limit_Delta_max,
                                            std::string &limit_UT_min,
                                            std::string &limit_UT_max,
                                            int &read_from_file_or_list_names_of_files,
                                            std::string &list_names_of_files,
                                            std::string &names_of_file_EQ
                                            )
/*
 *
 * Из http://mycpp.ru/cpp/book/c07.html
 * Альтернативой может стать объявление параметров ссылками. В данном случае реализация swap() выглядит так:

// rswap() обменивает значения объектов,
// на которые ссылаются vl и v2
void rswap( int &vl, int &v2 ) {
   int tmp = v2;
   v2 = vl;
   vl = tmp;
}
Вызов этой функции из main() аналогичен вызову первоначальной функции swap():

rswap( i, j );

7.3.1. Параметры-ссылки

Использование ссылок в качестве параметров модифицирует стандартный механизм передачи по значению.
При такой передаче функция манипулирует локальными копиями аргументов. Используя параметры-ссылки,
она получает l-значения своих аргументов и может изменять их.
В каких случаях применение параметров-ссылок оправданно?
Во-первых, тогда, когда без использования ссылок пришлось бы менять типы параметров на указатели (см. приведенную выше функцию swap()).
Во-вторых, при необходимости вернуть из функции несколько значений.
В-третьих, для передачи большого объекта типа класса.
 */

{
//Считаем файл инициилизирующих переменных для нашей работы
std::ifstream file(name_file_of_ini.toStdString().c_str());//создаем объект потока istream  по имени file
                        // который инициализируется  именем fileName,
                       //вызывается функция file.open();
std::string str;
QString q_string;
//QString str;
//переменная стринг для строки
//while(getline(qinClientFile, next_string, '\n'))
while(getline(file,str,'\n')) //getline(istream & is, string &s,char c='\n'),читает из потока is, в строку s пока
  {                        //не встретит символ c (без этого символа до новой строки)
                         // возвращает свой объект istream, в условии проверяется состояние iostate флагa,
                         // значение этого флага будет ложным, если достигнет конца файла, или будет ошибка ввода
                         // или читаемого типа
      q_string = QString::fromStdString(str);

      QStringList list_tmp = q_string.split("=", QString::SkipEmptyParts);
      if (list_tmp.at(0)=="do_dispers")
          do_dispers=list_tmp.at(1).toInt();
      if (list_tmp.at(0)=="limit_dispersion_min")
          limit_dispersion_min=list_tmp.at(1).toFloat();
      if (list_tmp.at(0)=="limit_dispersion_max")
          limit_dispersion_max=list_tmp.at(1).toFloat();
      if (list_tmp.at(0)=="limit_myltiply")
          limit_myltiply=list_tmp.at(1).toFloat();
      if (list_tmp.at(0)=="limit_sigma")
          limit_sigma=list_tmp.at(1).toFloat();
      if (list_tmp.at(0)=="multiply_y")
          multiply_y=list_tmp.at(1).toFloat();
      if (list_tmp.at(0)=="scale_T")
          scale_T=list_tmp.at(1).toFloat();
      if (list_tmp.at(0)=="add_multiply")
          add_multiply=list_tmp.at(1).toFloat();
      if (list_tmp.at(0)=="deriv")
          deriv=list_tmp.at(1).toInt();
      if (list_tmp.at(0)=="out_data_dir")
          out_data_dir=list_tmp.at(1).toStdString().c_str();
      if (list_tmp.at(0)=="dispers_data_file")
          dispers_data_file=list_tmp.at(1).toStdString().c_str();

      if (list_tmp.at(0)=="limit_Alpha_min")
          limit_Alpha_min=list_tmp.at(1).toFloat();
      if (list_tmp.at(0)=="limit_Alpha_max")
          limit_Alpha_max=list_tmp.at(1).toFloat();
      if (list_tmp.at(0)=="limit_Delta_min")
          limit_Delta_min=list_tmp.at(1).toFloat();
      if (list_tmp.at(0)=="limit_Delta_max")
          limit_Delta_max=list_tmp.at(1).toFloat();
      if (list_tmp.at(0)=="limit_UT_min")
          limit_UT_min=list_tmp.at(1).toStdString().c_str();
      if (list_tmp.at(0)=="limit_UT_max")
          limit_UT_max=list_tmp.at(1).toStdString().c_str();
      if (list_tmp.at(0)=="read_from_file_or_list_names_of_files")
          read_from_file_or_list_names_of_files=list_tmp.at(1).toInt();
      if (list_tmp.at(0)=="list_names_of_files")
          list_names_of_files=list_tmp.at(1).toStdString().c_str();
      if (list_tmp.at(0)=="names_of_file_EQ")
          names_of_file_EQ=list_tmp.at(1).toStdString().c_str();




      //fncn(str); // вызываем нужною функцию для полученной строки
  };
}


//Пишем файл сжатых данных (по умолчанию по 20 звездных секунд) - после калибровки и нахождения данных по всем лучам :
void grav::write_files_short_data_from_calb_data(std::string out_data_dir,
                                      std::string name_file_short,
                                      int device, int countsCanal, int n_bands,
                                      float resolution,
                                      const QString date_begin, // Дата начала регистрации файла в UTC, считано ранее из заголовка
                                      const QString time_begin, // Время начала регистрации файла в UTC, считано ранее из заголовка
                                      const float step_in_star_second,//шаг в звездных секундах
                                      const double star_time_in_Hour_start_file,
                                           //стартовая ЗВЕЗДНАЯ секунда НАЧАЛА часа (файла)
                                      const double star_time_start_end_H,
                                           //стартовая ЗВЕЗДНАЯ секунда часа,
                                           //с которой все начинается - посчитана для i=0 и не меняется !
                                      const double start_time_in_sec_end,
                                           //стартовая ВРЕМЕННАЯ секунда часа, с которой все начинается
                                           // - посчитана для i=0 и не меняется !
                                      const long start_point,
                                           // - именно на столько точек мы сдвинулись от начала файла при чтении данных из него,
                                           // считается при каждом новом i - ЗАНОВО
                                      const double mjd_start,
                                      const int counter_median_point_in_deriv,//точек в массивах
                                      const int i, //Номер считанного КУСКА-порции файла данных по counter_median_point_in_deriv
                                      double deriv_star,
                                           // количество сдвиговых секунд,вычисленных из ЗВЕЗДНЫХ секунд в порции данных
                                           //- реально их должно быть меньше, чтобы не было разрывов в данных
                                      std::vector<float> media_vec,
                                      std::vector<float> min_data_vec,
                                      std::vector<float> max_data_vec,
                                      std::vector<float> sigma_data_vec,
                                      std::vector<long> max_data_vec_index)
{
//Пишем файл сжатых данных (по умолчанию по 20 звездных секунд) - после калибровки и нахождения данных по всем лучам

    if (otladka==2)
              {
                 //str1 = now.toString;
             writeStream << "\n ZAPIS MEDIANN write_files_short_data_from_calb_data device,countsCanal,n_bands,resolution,="
                 << out_data_dir.c_str() << ","  //Запишет в файл параметры
                 << device << ",";
                 writeStream << countsCanal << " ,"
                             << n_bands << " ,"
                             << date_begin << " ,"
                             << time_begin << " ,"
                             << resolution << " step_in_star_second,star_time_in_Hour_start_file,star_time_start_end_H,start_time_in_sec_end,start_point,="
                             << step_in_star_second << " ,"
                             << star_time_in_Hour_start_file << " ,"
                             << star_time_start_end_H << " ,"
                             << start_time_in_sec_end << " ,"
                             << start_point << "; mjd_start,counter_median_point_in_deriv, i, deriv_star=="
                             << mjd_start << " ,"
                             << counter_median_point_in_deriv << " ,"
                             << i << " ,"
                             << deriv_star << " \n media_vec(0),min_data_vec(0),max_data_vec(0),sigma_data_vec(0),max_data_vec_index(0)=="
                             << media_vec.at(0) << " ,"
                             << min_data_vec.at(0) << ","
                             << max_data_vec.at(0) << " ,"
                             << sigma_data_vec.at(0) << " ,"
                             << max_data_vec_index.at(0) << " ,"
                             << "\n\n";
                 fileOut.flush();
               };//if (otladka==1)



    QString date_and_time=date_begin + " " + time_begin;
    QDateTime start_time_of_file = QDateTime::fromString(date_and_time, "dd.MM.yyyy HH:mm:ss");
    start_time_of_file.setTimeSpec(Qt::UTC);
               //Определили UTC старта файла
    QDateTime start_time_of_data = start_time_of_file;
    double d_t_in_sec=((star_time_start_end_H-star_time_in_Hour_start_file)*3600.0
                       +i*deriv_star+
                       step_in_star_second/2)
             /coeff_from_Tutc_to_Tstar;
    start_time_of_data = start_time_of_data.addMSecs(d_t_in_sec*1000);
    double mjd_start_inside=mjd_start+d_t_in_sec/86400.0;
    double star_time_start_end_H_inside=star_time_start_end_H+
            +(i*deriv_star*coeff_from_Tutc_to_Tstar)/3600.0+
            (step_in_star_second/2.0)/3600.0;
     //Сдвинули на середину диапазона звездное время

    QDateTime start_time_of_data_tmp = start_time_of_data;
    QString str_name_file, Qstr_tmp;
    Qstr_tmp=out_data_dir.c_str();
    int star_h,star_m,star_s;
    double mjd_start_inside_tmp=mjd_start_inside;
    double star_time_tmp=star_time_start_end_H_inside;

    //for(int i=0; i < counter_median_point_in_deriv; ++i)
    for (int cur_median_point = 0; cur_median_point < counter_median_point_in_deriv; cur_median_point++)
      {
       star_time_tmp=star_time_start_end_H_inside+(step_in_star_second*cur_median_point/3600.0);
       star_h=int(star_time_tmp);
       star_m=int((star_time_tmp-star_h)*60.0);
       if (star_m==60)
         {
           star_m=0;
           star_h=star_h+1;
           if (star_h==24)
             {
              star_h=0;
             };
         };
       star_s=int((star_time_tmp-star_h-star_m/60.0)*3600 +0.5);
       if (star_s==60)
         {
           star_s=0;
           star_m=star_m+1;
           if (star_m==60)
             {
               star_m=0;
               star_h=star_h+1;
               if (star_h==24)
                 {
                  star_h=0;
                 };
             };
         };

       str_name_file=Qstr_tmp + (star_h < 10 ? "0" : "") + QString::number(star_h)
            + (star_m < 10 ? "0" : "") + QString::number(star_m)
            + (star_s < 10 ? "0" : "") + QString::number(star_s)
            + "_N"+ QString::number(device) +
            + "_B"+ QString::number(n_bands) +
            + "_" + start_time_of_data_tmp.toString("yyyyMMddhhmmss")+".txt";

       d_t_in_sec=step_in_star_second*cur_median_point/coeff_from_Tutc_to_Tstar;
       start_time_of_data_tmp = start_time_of_data;
       start_time_of_data_tmp = start_time_of_data_tmp.addMSecs(d_t_in_sec*1000);
       mjd_start_inside_tmp=mjd_start_inside+d_t_in_sec/86400.0;


       QFile fileOut_RA_data(str_name_file);
       // Связываем объект с файлом fileout.txt
       QTextStream writeStream(&fileOut_RA_data); // Создаем объект класса QTextStream
        // и передаем ему адрес объекта fileOut
       QString str1,str_tmp;


       if(fileOut_RA_data.open(QIODevice::WriteOnly | QIODevice::Text))
          { // Если файл успешно открыт для записи в текстовом режиме
            writeStream.setCodec(QTextCodec::codecForName("cp1251"));

            //str_tmp = QString("%1").arg(mjd_start_inside_tmp, 0, 'f', 8);

            str1 =name_file_short.c_str();
            str1 =str1 + "|UT=" + start_time_of_data_tmp.toString("yyyy-MM-dd hh:mm:ss.zzz") +
                   "|MJD=" + QString("%1").arg(mjd_start_inside_tmp, 0, 'f', 8) +
                   "|RA=" + QString("%1").arg(star_time_tmp, 0, 'f', 4);
            //str1 = date.toString("dd.MM.yyyy ")+time.toString("hh:mm:ss.zzz");

            for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)
             //Проход по лучам БСА - пишем среднемедианные и прочие значения всех лучей
              {
                str1 = str1 + "|{";
                for (unsigned int currentBand = 0; currentBand <= n_bands; currentBand++)
                  {//Проход по ВСЕМ полосам (в том числе - ОБЩЕЙ) - пишем среднемедианные и прочие значения 1 луча

                   str1 = str1 + QString("%1").arg(media_vec[currentCanal*(n_bands+1)*counter_median_point_in_deriv
                           +currentBand*counter_median_point_in_deriv+cur_median_point], 0, 'f', 4) + ",";
                   //МЕДИАННОЕ: Записываем 4 знака после запятой, в нотации без степеней :  'f', 4
                   //По: http://doc.crossplatform.ru/qt/4.7.x/qstring.html
                   str1 = str1 + QString("%1").arg(min_data_vec[currentCanal*(n_bands+1)*counter_median_point_in_deriv
                           +currentBand*counter_median_point_in_deriv+cur_median_point], 0, 'f', 4) + ",";
                   //Минимальное
                   str1 = str1 + QString("%1").arg(max_data_vec[currentCanal*(n_bands+1)*counter_median_point_in_deriv
                           +currentBand*counter_median_point_in_deriv+cur_median_point], 0, 'f', 4) + ",";
                   //максимальное
                   str1 = str1 + QString("%1").arg(max_data_vec_index[currentCanal*(n_bands+1)*counter_median_point_in_deriv
                           +currentBand*counter_median_point_in_deriv+cur_median_point], 0, 'f', 0) + ",";
                     //ИНДЕКС Максимального - от начала часа-файла, Записываем в виде целого числа, т.е. 0 знаков после запятой:  'f', 0
                   str1 = str1 + QString("%1").arg(sigma_data_vec[currentCanal*(n_bands+1)*counter_median_point_in_deriv
                           +currentBand*counter_median_point_in_deriv+cur_median_point], 0, 'f', 4) ;
                    //Дисперсия (после отброса 30% верхних и 4% нижних значений. Она примерно в 2 раза меньше, чем реальная)
                   if (currentBand==n_bands)
                     {
                      str1 = str1 + "}";
                     }
                   else
                     {
                      str1 = str1 + ",";
                     };
                  };//for (unsigned int currentBand = 0; currentBand <= n_bands; currentBand++)
                /*
                if (currentCanal==(countsCanal-1))
                  {
                   str1 = str1 + "|";
                  };
                  */
              };//for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)


            writeStream << str1 ;
                     // Посылаем строку в поток для записи
            fileOut_RA_data.flush();
            fileOut_RA_data.close(); // Закрываем файл
          };//if(fileOut_RA_data.open(QIODevice::WriteOnly | QIODevice::Text))


      }; //for(int i=0; i < counter_median_point_in_deriv; ++i)
     //for(int i=0; i < counter_median_point_in_deriv; ++i)


}

//Функция для отрисовки данных в графический файл - с входными параметрами:
void grav::runSaveFigure_in_File(int band_or_ray, float add_tmp, double koeff,
                                 //const QDateTime now, //время UT начала данных
                                 double /* start_time_in_sec */ start_time_in_sec_end,
                                 const long start_point,
                                 double multiply_y, double scale_T, double add_multiply,
                                 float srednee_all_band,
                                 const std::vector<float> &test_vec,//Начальные сложенные данные , без учета дисперсий
                                 std::vector<float> &test_vec_end,
                                  //Конечные сложенные данные , часть вектора - для лучшей дисперсии
                                 const std::vector<float> &test_vec_end_tmp,
                                  //Конечные сложенные данные , накопленные максимальные по разным дисперсиям
                                 Point_Impuls_D &Impuls)
/*
 * band_or_ray =0 - рисуем по полосам (т.е. максимуми 32 линии данных на рисунке)
 *         или =1 по лучам диаграммы (до 48)
 * double koeff - масштабный коэффициент графиков, по умолчанию = 400
 * start_time_in_sec - стартовая секунда, с которой надо отрисовывать файлы порций; Было:
 *                     double start_time_in_sec = ui->doubleSpinBox->value();
 *      // int start_point =(int) ( start_time_in_sec / (0.001 * bsaFile.header.tresolution) +0.5);
 * multiply_y  - масштабный коэффициент графика по Y (чем больше, тем крупнее масштаб)
 *          было ранее multiply_y = ui->doubleSpinBox_2->value()
 * scale_T - масштабный отрезок температуры, который пририсовывается справа внизу по правой оси Y
 * (и на данных впритык к ней)
 *    ,было ранее: scale_T = ui->doubleSpinBox_3->value();
 * add_multiply - сжимающий графики по оси Y коэффициент - втискивает графики из многих строк в окно
 *                   было ранее: add_multiply=ui->doubleSpinBox_4->value();
 *
 */
{

    int resolution=Impuls.resolution;

    double  curTime_in_ms;
    long curTime_in_ms_int;

    float y_first,  y_second, add_channel_in_graph, y_metka, tmp_float; //переменные для тестов
    float array_1_mem[10], array_2_mem[10];
           //массивы в память для тестов - туда кладем с векторов что-либо для вывода в режиме отладки
    add_channel_in_graph=0;// > 0 - сдвиг на определенное число каналов вверх всего рисунка
    unsigned int currentBand;
    unsigned int i;
    double calib_data_T; //Переменная для хранения текущих откалиброванных данных (в Кельвинах)
    int otladka_add_1=0; //Для добавочного отладочного коэффициента, который будет употреблен в калибровочной функции
    double temp; //Для запоминания температуры (после калибровки) предпоследней точки последнего (нижнего) луча

    QString OutLine_1; //для формирования меток на оси

    //PlotSettings settings;
    //График рисуем

    int size_point = bsaFile.data_file.size() / bsaFile.header.npoints;
      //Число данных в 1 временной точке, для 6 полос и 48 лучей =336

    QDateTime now = QDateTime::fromString(QString("01.01.1970") + ((int)pStarTime->hour < 10 ? "0" : "")
                                          + QString::number((int)pStarTime->hour) + ":"
                                          + ((int)pStarTime->minute < 10 ? "0" : "")
                                          + QString::number((int)pStarTime->minute) + ":"
                                          + ((int)pStarTime->second < 10 ? "0" : "")
                                          + QString::number((int)pStarTime->second, 'f', 0), "dd.MM.yyyyhh:mm:ss");
    now.setTimeSpec(Qt::UTC);
    //Выставили время на начало эпохи UNIX (из звездного)...

    koeff = 400.0;

    unsigned int countsCanal = bsaFile.header.modulus.size() * bsaFile.nCanal;
     // количество каналов - точнее, ЛУЧЕЙ БСА, покрываемых файлом. Обычно равно 48
    double tresolution = 0.001 * bsaFile.header.tresolution;
     //реальное временнОе разрешение по 1 точке - в секундах


    QVector< QVector<QPointF>> curve_vecs0, curve_vecs1, curve_vecs2;
      //Класс QPointF описывает точку на плоскости, используя точность плавающей запятой.
      //                                    http://doc.crossplatform.ru/qt/4.5.0/qpointf.html
      //Точка определяется координатой x и координатой y, которые могут быть доступны используя функции x() и y().
      //Координаты точки определяются используя точность чисел с плавающей запятой.

    QMap<double, QString> listY; // список меток по оси Y (слева)
    QMap<double, QString> listY_2; // список меток по оси Y (справа)
    float array_metka_Y2[countsCanal];// МАССИВ меток по оси Y (справа) - по-лучам

    std::string tmp_string; //(Самодуров В.А.)
    // tmp_string=bsaFile.header.date_begin.toStdString().c_str() + "UTC";
    std::string tmp_str_1="UTC  ";
    tmp_string=tmp_str_1 + bsaFile.header.date_begin.toStdString().c_str();
    /*
       Функция c_str() возвращает указатель на символьный массив,
       содержащий строку объекта string в том виде,
       в каком она находилась бы во встроенном строковом типе.
    */
    const char *str_X = tmp_string.c_str(); // правильно, теперь в ней содержится нижняя подпись

    if (otladka==1)
     {
        //str1 = now.toString;

        writeStream << "\n runSaveFigure_in_File , input param: koeff,multiply_y,scale_T, add_multiply= "
                    << koeff << "," << multiply_y <<  ","  << scale_T << ","  << add_multiply << "\n";
                    //Запишет в файл параметры
        writeStream << "Input param: start_time_in_sec_end,  start_point= "
                    << start_time_in_sec_end << ","  << start_point << "\n";
                    //Запишет в файл параметры
        fileOut.flush();
      };//if (otladka==1)

    /*
     * multiply_y, double scale_T, double add_multiply
     */

    if(band_or_ray == 1) //ПОКА НЕ ИСПОЛЬЗУЮ!
    {
        //работаем по полосам, для каждой полосы частот создаем свою картинку по 48 лучей
        unsigned int startBand = 0;
        unsigned int finishBand = bsaFile.header.nbands;

        if(ui->comboBox->currentIndex() != 0)
            startBand = finishBand = ui->comboBox->currentIndex()-1;
        else if(ui->checkBox->isChecked()) //Чек-бокс "Сдвиг"
        {
            curve_vecs0.reserve(countsCanal * bsaFile.header.npoints);
            curve_vecs1.reserve(countsCanal * bsaFile.header.npoints * bsaFile.header.nbands);
        }

        for (currentBand = startBand; currentBand <= finishBand; currentBand++)
         {//Одну за другой создаем 48-ми лучевые картинки ДЛЯ КАЖДОЙ ПОЛОСЫ
            widget_2.clearCurves();

            for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)
                //Проход по лучам БСА
              {
                QVector<QPointF> curve_vec(bsaFile.header.npoints);
                total = 0.0;

                for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
                   //Проход по временнЫм точкам
                  {

                    //!!!!!!!!
                    if ( ((otladka==1) && (currentCanal==countsCanal-1))
                             && ( i==bsaFile.header.npoints-1 ) )
                      {
                        otladka_add_1=1;


                        int tmp_i=size_point * i
                                + currentCanal * (bsaFile.header.nbands+1)
                                + currentBand;
                        writeStream << "runSaveFigure_in_File: size_point,i,Canal,nbands,currentBand , tmp_i, y= "
                                    << size_point << "," << i << "," << currentCanal << "," << bsaFile.header.nbands
                                    << "," << currentBand << "," << tmp_i
                                    << "," << bsaFile.data_file[tmp_i]
                                    << "\n"; //Запишет в файл параметры

                      }//if (otladka==1)
                    else
                      {
                       otladka_add_1=0;
                      };


                    /* Не надо калибровать в функции runSaveFigure_in_File , все уже сделано до входа в нее!
                    calib_data_T=calib(bsaFile.data_file[size_point * i //(i+start_point)
                            + currentCanal * (bsaFile.header.nbands+1)
                            + currentBand],//
                            tresolution * i, currentBand, currentCanal, otladka_add_1);
                    //Откалибровали к градусам Кельвина значение данных
                    otladka_add_1=0;
                    //Сбросили на всякий случай значение вывода отладочных значений внутри калибровки к НЕ-выводу

                    */

                    if((i==bsaFile.header.npoints-1) && (currentCanal==countsCanal-1 ))
                     //Захватываем в значение метки на правой оси графика значение последней точки последнего (нижнего) канала
                     {
                       temp=calib_data_T;
                       // !!!!!!!!!!!!! Переработать! calib_data_T - еще не присвоено!
                     };

                    curve_vec[i] = QPointF(tresolution *i + curTime,  calib_data_T / koeff);
                    //Приняли значение вектора на графике (поделив на масштаьных коэффициент

                    if ( ((otladka==1) && (currentCanal==countsCanal-1) && (currentBand==startBand))
                         && ((i==0) || (i==bsaFile.header.npoints-1)) )
                     {
                        writeStream << "band,ray,i,calb_T= "
                                    << currentBand << ", " << currentCanal << ", " << i << ", "
                                    << calib_data_T << "\n"; //Запишет в файл параметры
                      };//if (otladka==1)
                  };
                  //Проход по временнЫм точкам for (unsigned int i=0; i < bsaFile.header.npoints; ++i)

                double y_average = total / (koeff * bsaFile.header.npoints);

                for (i=0; i < bsaFile.header.npoints; ++i)
                   {
                    if((i==bsaFile.header.npoints-2) && (currentCanal==countsCanal-1 ))
                     {
                       add_tmp=scale_T/koeff ;   /* 100.0/koeff */
                       y_first=curve_vec[i].y();
                       curve_vec[i].setY( (curve_vec[i].y() + add_tmp - y_average)*multiply_y
                                         + (countsCanal+add_channel_in_graph - currentCanal) * add_multiply);
                       y_second=curve_vec[i].y();
                       array_1_mem[0]=y_first;
                       array_1_mem[1]=(y_first - y_average)*multiply_y +
                               (countsCanal+add_channel_in_graph - currentCanal)  * add_multiply;
                       array_1_mem[2]=(y_first + add_tmp - y_average)*multiply_y +
                               (countsCanal+add_channel_in_graph - currentCanal) * add_multiply;

                       y_metka=curve_vec[i].y();
                       OutLine_1="+" + OutLine_1.setNum(scale_T) + " (10~1Jy)";

                       // !!! settings.mListY2.insert(y_metka,OutLine_1);

                       listY_2.insert(y_metka, OutLine_1);
                       //Метки на оси Y справа
                       //Вставили верхнюю метку (соответствует сдвинутому вверх значению)
                     }
                    //Искуственно добавляем 100 К (нет, теперь scale_T !) чтобы вывести реальный масштаб!
                    else
                     {
                       curve_vec[i].setY( (curve_vec[i].y() - y_average)*multiply_y +
                                          (countsCanal+add_channel_in_graph - currentCanal)* add_multiply);
                     };

                   }; //(unsigned int i=0; i < bsaFile.header.npoints; ++i)

                // !!!!!!!!!!!!!!!!!!!!!!!
                if ((otladka==1) && (currentCanal==countsCanal-1) && (currentBand==startBand))
                 {
                    //str1 = now.toString;
                    //float tmp1=curve_vec[0];
                    writeStream << "IN runSaveFile, currentCanal, i, curve_vec[0],curve_vec[bsaFile.header.npoints-1],npoints= "
                                << currentCanal << "," << i << ","  << curve_vec[0].y() << "," << curve_vec[bsaFile.header.npoints-1].y()
                               << "," << bsaFile.header.npoints << "\n"; //Запишет в файл параметры
                    fileOut.flush();
                  };//if (otladka==1)

                //y_second=y_second; // Отладочная строка
                widget_2.addCurve(curve_vec,
                                  QColor(255 - 255 / bsaFile.header.nbands * currentBand,
                                         255 / (4*bsaFile.header.nbands) * currentBand,
                                         255 / bsaFile.header.nbands * currentBand)); // Qt::blue);

                listY.insert(curve_vec[0].y(), ui->comboBox_2->itemText(currentCanal+1));
                //Метки на оси Y слева ()1- число - Y, второе - строка формата :
                //взяли Y от 0-го элемента curve_vec, а текст метки сгененировали при помощи itemText(currentCanal+1)

                //формируем правую метку на последней точке канала
                array_metka_Y2[currentCanal] = curve_vec[bsaFile.header.npoints-1].y();

                if ((otladka==1) && (currentCanal==countsCanal-1) && (currentBand==startBand))
                 {
                    //str1 = now.toString;
                    //float tmp1=curve_vec[0];
                    writeStream << "IN METKA , currentBand,last-1,last= "
                                << currentBand << "," << curve_vec[bsaFile.header.npoints-2].y() << "," << curve_vec[bsaFile.header.npoints-1].y()
                                << "\n"; //Запишет в файл параметры
                    fileOut.flush();
                  };//if (otladka==1)

              };
              //Проход по лучам БСА for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)

            PlotSettings settings;
            //settings.setAxis("X", curTime, curTime + tresolution * bsaFile.header.npoints, "UTC");
             //выставляем метки по времени UTC снизу

            settings.setAxis("X", curTime, curTime + tresolution * bsaFile.header.npoints,str_X);
             //оттолкнулись от секунд UNIX

            settings.setAxis("Y", 0.0, countsCanal);
            settings.setAxis("X2", now.toTime_t(),
                             now.toTime_t() + tresolution * bsaFile.header.npoints * 86164.090530833/86400.0,
                             "RA", true, Qt::darkGreen);
             //выставляем метки по времени RA сверху - оттолкнулись от звездного времени начала pStarTime
            settings.setAxis("Y2", 0.0, countsCanal, "", true);


            array_2_mem[0]=total;
            if ((otladka==1) && (otladka_add_1==1))
             {
                //str1 = now.toString;
                //float tmp1=curve_vec[0];
                writeStream << "PERED CALB TEMP: countsCanal, nbands,finishBand , y= "
                            << currentBand << "," << bsaFile.header.nbands << "," << finishBand
                            << "," << bsaFile.data_file[(countsCanal-1) * (bsaFile.header.nbands+1) + finishBand]
                            << "\n"; //Запишет в файл параметры
                fileOut.flush();
              };//if (otladka==1)

           /* Внимание! Правильный temp захвачен выше, а перевычисление - на самом деле было неверным, закомментарили.
            temp = calib(bsaFile.data_file[(countsCanal-1) * (bsaFile.header.nbands+1) + finishBand],
                    0.0, currentBand, countsCanal-1,otladka_add_1);
           */
            array_2_mem[1]=temp;
            total=array_2_mem[0];
            //Восстанавливаем искаженное лишней калибровкой значение total


            //Возьмем крайнее правое значение по Y последнего (нижнего) луча - как начальную метку 0 ,
            //но с правильным значением - для правой оси
            OutLine_1="T=" + OutLine_1.setNum(temp) + " K";

            y_metka=array_metka_Y2[countsCanal-1];

            listY_2.insert(y_metka, OutLine_1);
            //Метки на оси Y справа
            //Вставили нижнюю метку (соответствует значению последней точки)

            settings.autoAdjust = false;
            settings.mTickLabelTypeX = PlotSettings::mtDateTime;
            settings.mDateTimeFormatX = "hh:mm:ss";

#ifdef Q_OS_WIN
        settings.MarginX = 80;
#endif

#ifdef Q_OS_LINUX
        settings.MarginX = 100;
#endif

            settings.autoAdjustY = false;
            settings.mTickLabelTypeY = PlotSettings::mtList;
            settings.mListY.swap(listY);
            settings.numYTicks = settings.mListY.count() + 1;

            settings.autoAdjustY2 = false;
            //!!! -
            settings.mListY2.swap(listY_2);

            settings.numY2Ticks = 2;//1

            if(currentBand == bsaFile.header.fbands.size())
                settings.title = "Частота усреднённая (109.0-111.5 МГц)";
            else
                settings.title = "Частота " + QString::number(bsaFile.header.fbands[currentBand],'f', 3) + " МГц";

            widget_2.setPlotSettings(settings);

            if(!ui->widget->isHidden())
                widget_2.update();

            // сохранение в файл
            if(!ui->checkBox->isChecked())
                widget_2.save(ui->leDir->text() + QString::number(mjd_start, 'f', 5) + "_"
                              + QString::number(currentBand+1) + "_band.png", "PNG", 1200, 600);

            // сохранение со сдвигом  - УБРАНО
         };
        //for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)
        //Закончили создание 48-ми лучевых картиноки ДЛЯ КАЖДОЙ ПОЛОСЫ

        // !!!!!!!!!!!!!!!!!!!!!!!
        if (otladka==2)
         {
            //str1 = now.toString;

            writeStream << "\n IN runSaveFile, posle polos  startBand,currentBand,total= "
                        << startBand <<  ","  << currentBand << "," << total << "\n"; //Запишет в файл параметры
            fileOut.flush();
          };//if (otladka==1)

        // сохранение со сдвигом  - УБРАНО
    }
    //if(band_or_ray == 1)
    else
    {
        // по лучам (для луча создаем картинку - проход по частотам снизу вверх, 33 частоты)
        //unsigned int currentCanal = ui->comboBox_2->currentIndex()-1;
        unsigned int currentCanal =Impuls.ray;
         //номер канала нашли

        QDateTime UT_start=Impuls.UT; //время UT начала рисунка
        int ind_max=Impuls.ind-start_point;
        int ind_start=ind_max-int(2.0*Impuls.d_T_Half_H*1000/Impuls.resolution + 0.5);
        if (ind_start<0)
          {ind_start=0;};
        int ind_finish=ind_max+int(30.0*1000/Impuls.resolution +0.5);
        if (ind_finish>bsaFile.header.npoints)
          {ind_finish=bsaFile.header.npoints;};

        int nn_points=ind_finish-ind_start+1;

        if (otladka==1)
         {
            writeStream << "runSaveFigure, show bands: ind_start,ind_max,ind_finish= "
                        << ind_start << "," << ind_max << "," << ind_finish
                        << "\n"; //Запишет в файл параметры
            fileOut.flush();
          };//if (otladka==1)

        //UT_start = UT_start.addMSecs((-Impuls.d_T_Half_H)*1000);
        UT_start = UT_start.addMSecs((ind_start-ind_max)*Impuls.resolution);
        curTime = UT_start.toTime_t();
           //Текущее время - взяли как UTC,превратили в  curTime  - это UNIX секунды, пойдет на нижнюю ось

        //double  curTime_in_ms;
        //long curTime_in_ms_int;
        curTime_in_ms=UT_start.toMSecsSinceEpoch()/1000.0;
        QString str;
        QStringList list_tmp;

        str = UT_start.toString("yyyy:MM:dd:hh:mm:ss.zzz");
        list_tmp= str.split(":", QString::SkipEmptyParts);
        //std::vector<std::string> str_vec_tmp = split(str, ':');
        starTime curStarTime(list_tmp.at(0).toInt(),
                             list_tmp.at(1).toInt(),
                             list_tmp.at(2).toInt(),
                             list_tmp.at(3).toInt(),
                             list_tmp.at(4).toInt(),
                             /* list_tmp.at(5).toInt() - неверно!!! -->0 ! */ list_tmp.at(5).toDouble() ,
                             /*2.50866666667*/ 2.50875326667, 0);
        /*
         * starTime(int Year = 2000, int Month = 1, int Date = 1, int hr = 0, int min = 0, double sec = 0,
             double longitude_in_h = 2.50875326667, int d_h_from_UT = -4)
             ...
           hour = floor(Star_in_real_H);//ceil округляет ВВЕРХ до  ближайшего целого, а вот floor - ВНИЗ, и де-факто равно div из Паскаля
           double tmp_dat = (Star_in_real_H-hour)*60;
           minute = floor(tmp_dat);
           second = (tmp_dat-minute)*60;
         */

          //starTime - это класс, вычисляющий звездное время для предложенных параметров (см. qbsa.h)

          //посчитали звездное время старта отрисовки из UTC и долготы Пущино
        pStarTime = &curStarTime;// - и захватили его в указатель

        /*
        now = QDateTime::fromString(QString("01.01.1970") + ((int)pStarTime->hour < 10 ? "0" : "")
                                                  + QString::number((int)pStarTime->hour) + ":"
                                                  + ((int)pStarTime->minute < 10 ? "0" : "")
                                                  + QString::number((int)pStarTime->minute) + ":"
                                                  + ((int)pStarTime->second < 10 ? "0" : "")
                                                  + QString::number(pStarTime->second, 'f', 3),
                                    "dd.MM.yyyyhh:mm:ss.zzz"); */

        now = QDateTime::fromString(QString("01.01.1970") +
                     ((int)curStarTime.hour < 10 ? "0" : "") + QString::number((int)curStarTime.hour) + ":" +
                     ((int)curStarTime.minute < 10 ? "0" : "") + QString::number((int)curStarTime.minute) + ":" +
                     ((int)curStarTime.second < 10 ? "0" : "") + QString::number((int)curStarTime.second, 'f', 3),
                      "dd.MM.yyyyhh:mm:ss.zzz");
        now.setTimeSpec(Qt::UTC); //получить значение из этой переменной в определённом часовом поясе

        //now.setTimeSpec(Qt::UTC);
            //Выставили время на начало эпохи UNIX (из звездного)...

        QString new_str = now.toString("yyyy:MM:dd:hh:mm:ss.zzz");


        if (otladka==1)
         {
            //str1 = now.toString;
            //float tmp1=curve_vec[0];
            writeStream << "Start_UT , Start_STAR, curTime, curTime_in_ms= "
                        << str << ","
                        << new_str << "," << curTime << "," << curTime_in_ms
                        << "\n"; //Запишет в файл параметры
            writeStream << "Start_STAR in part: hour,minute,second= "
                        << curStarTime.hour << ","
                        << curStarTime.minute << "," << curStarTime.second
                        << "\n"; //Запишет в файл параметры
            writeStream << "Start_UT in part: yyyy,MM,dd,hour,minute,second_int; sec= "
                        << list_tmp.at(0).toInt() << ","
                        << list_tmp.at(1).toInt() << ","
                        << list_tmp.at(2).toInt() << ","
                        << list_tmp.at(3).toInt() << ","
                        << list_tmp.at(4).toInt() << ","
                        << list_tmp.at(5).toInt() << "; "
                        << list_tmp.at(5)
                        << "\n";
            fileOut.flush();
          };//if (otladka==1)

        widget_2.clearCurves();
        for (unsigned int currentBand = 0; currentBand < bsaFile.header.nbands; currentBand++)
        {
            QVector<QPointF> curve_vec(nn_points);
            total = 0.0;

            /* !!!!!!!!!!! Было ранее.
            for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
                curve_vec[i] = QPointF(tresolution * i + curTime,
                                       calib(bsaFile.data_file[size_point * i + currentCanal * (bsaFile.header.nbands+1) + currentBand],
                                       tresolution * i, currentBand, currentCanal,otladka_add_1) / koeff + currentBand + 1);

            double y_average = total / (koeff * bsaFile.header.npoints);


            for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
                curve_vec[i].setY((curve_vec[i].y() - y_average)*multiply_y);
                */

            //for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
            for (unsigned int i=ind_start; i <= ind_finish; ++i)
            {
               /* curve_vec[i-ind_start] = QPointF(tresolution * (i-ind_start) + curTime,
                                       bsaFile.data_file[size_point * (i) + currentCanal * (bsaFile.header.nbands+1)
                                          + currentBand]
                                      / koeff + (currentBand + 1) * add_multiply);
                                      */ //Исправил 12.05.2017 - чтобы исправить тонкий сдвиг графика - на нижнюю строку:
                curve_vec[i-ind_start] = QPointF(tresolution * (i-ind_start) + curTime_in_ms,
                                       bsaFile.data_file[size_point * (i) + currentCanal * (bsaFile.header.nbands+1)
                                          + currentBand]
                                      / koeff + (currentBand + 1) * add_multiply);

            };

            /*
             * Для лучей:

             calib_data_T=calib(bsaFile.data_file[size_point * i
                            + currentCanal * (bsaFile.header.nbands+1)
                            + currentBand],//
                            tresolution * i, currentBand, currentCanal, otladka_add_1);
                    //Откалибровали к градусам Кельвина значение данных

            curve_vec[i] = QPointF(tresolution *i + curTime,  calib_data_T / koeff);
                    //Приняли значение вектора на графике (поделив на масштаьных коэффициент
            curve_vec[i].setY( (curve_vec[i].y() - y_average)*multiply_y +
                               (countsCanal+add_channel_in_graph - currentCanal)* add_multiply);
                               при этом:
                               add_channel_in_graph=0;// > 0 - сдвиг на определенное число каналов вверх всего рисунка
            */

            //for (unsigned int i=0; i < bsaFile.header.npoints; ++i)
            for (unsigned int i=ind_start; i <= ind_finish; ++i)
               curve_vec[i-ind_start].setY(curve_vec[i-ind_start].y()*multiply_y);

            widget_2.addCurve(curve_vec, QColor(255 - 255 / bsaFile.header.nbands * currentBand,
                              255 / (4*bsaFile.header.nbands) * currentBand, 255 / bsaFile.header.nbands * currentBand));
                                   // Qt::blue);
            //Отрисовали обычные полосы-каналы - обычно 32.

            if ((otladka==2) && (currentBand ==0))
             {
                writeStream << "nn_points, Size_curve_vec , curve_vec_x.[0],[1], [Size-2],[Size-1] = "
                            << nn_points << ","
                            << curve_vec.size() << ","
                            << curve_vec[0].x() << ","
                            << curve_vec[1].x() << ","
                            << curve_vec[curve_vec.size()-2].x() << ","
                            << curve_vec[curve_vec.size()-1].x()
                            << "\n"; //Запишет в файл параметры
                fileOut.flush();
              };//if (otladka==1)
            // !!! -
            listY.insert(curve_vec[0].y(), QString::number(currentBand+1) + " ("
                    + QString::number(currentBand == bsaFile.header.nbands ? (*bsaFile.header.fbands.begin()
                    + *bsaFile.header.fbands.rbegin())/2 : bsaFile.header.fbands[currentBand], 'g', 6) + "М)");
            //Подписали слева на оси номера частот и частоту в мегагерцах

            if (otladka==2)
             {
                tmp_float=curve_vec[0].y();
                writeStream << "Vstavka sleva currentBand, curve_vec[0].y(), y_metka_left= "
                            << currentBand << "," << tmp_float << ","
                            << "Frec"
                            << "\n"; //Запишет в файл параметры
                fileOut.flush();
              };//if (otladka==1)

            //if (currentBand==bsaFile.header.nbands-1)
            if (currentBand==0)
            {
                /*
                settings.mListY2.insert(1000.0 * ( listY.lastKey() - (bsaFile.header.nbands+1) )
                                / ( calib(bsaFile.data_file[currentCanal * (bsaFile.header.nbands+1) + bsaFile.header.nbands],
                                  0.0, bsaFile.header.nbands, currentCanal,otladka_add_1) - total / nn_points
                                         ),
                                 "1000K ~ 100Jy");
                                 */

                //Возьмем крайнее правое значение по Y последнего (нижнего) луча - как начальную метку 0 ,
                //но с правильным значением - для правой оси

                y_metka=
                  // curve_vec[i].y();
                //=curve_vec[i-ind_start] =
                        /*
                        QPointF(tresolution * (ind_finish-ind_start) + curTime_in_ms,
                                                  (bsaFile.data_file[size_point * (ind_finish)
                                                   + currentCanal * (bsaFile.header.nbands+1)+ currentBand])
                                                 / koeff + (currentBand + 1) * add_multiply);
                                                         */
                        (bsaFile.data_file[size_point * (ind_finish)
                         + currentCanal * (bsaFile.header.nbands+1)+ currentBand])
                       / koeff + (currentBand + 1) * add_multiply;

                OutLine_1="T=" + OutLine_1.setNum(bsaFile.data_file[size_point * (ind_finish)
                        + currentCanal * (bsaFile.header.nbands+1)+ currentBand]) + " K";

                listY_2.insert(y_metka, OutLine_1);
                //Метки на оси Y справа
                //Вставили нижнюю метку (соответствует значению последней точки)

                if (otladka==2)
                 {
                    //str1 = now.toString;
                    //float tmp1=curve_vec[0];
                    tmp_float=curve_vec[ind_finish-ind_start].y();
                    writeStream << "Vstavka curve_vec[ind_finish], y_metka_2_1 , OutLine_1= "
                                << tmp_float << ","
                                << y_metka << ","
                                << OutLine_1
                                << "\n"; //Запишет в файл параметры
                    fileOut.flush();
                  };//if (otladka==1)


                y_metka=
                  // curve_vec[i].y();
                //=curve_vec[i-ind_start] =
                    /*    QPointF(tresolution * (ind_finish-ind_start) + curTime_in_ms,
                                                  (bsaFile.data_file[size_point * (ind_finish)
                                                   + currentCanal * (bsaFile.header.nbands+1)+ currentBand]
                                                   +scale_T)
                                                 / koeff + (currentBand + 1) * add_multiply);
                                                         */
                        (bsaFile.data_file[size_point * (ind_finish)
                         + currentCanal * (bsaFile.header.nbands+1)+ currentBand]
                         +scale_T)
                       / koeff + (currentBand + 1) * add_multiply;
                OutLine_1="+" + OutLine_1.setNum(scale_T) + " (10~1Jy)";

                listY_2.insert(y_metka, OutLine_1);

                if (otladka==2)
                 {
                    writeStream << "Vstavka  curve_vec[ind_finish], y_metka_2_2 , OutLine_1= "
                                << tmp_float << ","
                                << y_metka << ","
                                << OutLine_1
                                << "\n"; //Запишет в файл параметры
                    fileOut.flush();
                  };//if (otladka==1)

            } // if (currentBand==0)

        }//for (unsigned int currentBand = 0; currentBand < bsaFile.header.nbands; currentBand++)


        //for (unsigned int currentBand = 0; currentBand < bsaFile.header.nbands; currentBand++)
        {
            unsigned int currentBand =  bsaFile.header.nbands;
            QVector<QPointF> curve_vec(nn_points);

            /*

             float srednee_all_band,
             const std::vector<float> &test_vec,
                 //Начальные сложенные данные , без учета дисперсий
             std::vector<float> &test_vec_end,
                 //Конечные сложенные данные , часть вектора - для лучшей дисперсии
             const std::vector<float> &test_vec_end_tmp,
                //Конечные сложенные данные , накопленные максимальные по разным дисперсиям

             */

            for (unsigned int i=ind_start; i <= ind_finish; ++i)
               {
                /* curve_vec[i-ind_start] = QPointF(tresolution * (i-ind_start) + curTime,
                                       (test_vec.at(i)-srednee_all_band)
                                      / koeff + (currentBand + 2) * add_multiply); */

                //Исправил 12.05.2017 - чтобы исправить тонкий сдвиг графика - на нижнюю строку:
                curve_vec[i-ind_start] = QPointF(tresolution * (i-ind_start) + curTime_in_ms,
                                           (test_vec.at(i)-srednee_all_band)
                                            / koeff + (currentBand + 2) * add_multiply);
               };


            for (unsigned int i=ind_start; i <= ind_finish; ++i)
               curve_vec[i-ind_start].setY(curve_vec[i-ind_start].y()*multiply_y);

           // !! widget_2.addCurve(curve_vec, QColor(0,0,255) );
                                   // Qt::blue);

            /*
            listY.insert(curve_vec[0].y(), QString::number(currentBand+1) + " ("
                    + QString::number(currentBand == bsaFile.header.nbands ? (*bsaFile.header.fbands.begin()
                    + *bsaFile.header.fbands.rbegin())/2 : bsaFile.header.fbands[currentBand], 'g', 6) + "М)");
            //Подписали слева на оси номера частот и частоту в мегагерцах
            */


            for (unsigned int i=ind_start; i <= ind_finish; ++i)
            {
                /* curve_vec[i-ind_start] = QPointF(tresolution * (i-ind_start) + curTime,
                                               (test_vec_end_tmp.at(i)-srednee_all_band)
                                              / koeff + (currentBand + 2) * add_multiply); */
                //Исправил 12.05.2017 - чтобы исправить тонкий сдвиг графика - на нижнюю строку:
                curve_vec[i-ind_start] = QPointF(tresolution * (i-ind_start) + curTime_in_ms,
                                           (test_vec_end_tmp.at(i)-srednee_all_band)
                                            / koeff + (currentBand + 2) * add_multiply);
            };
            for (unsigned int i=ind_start; i <= ind_finish; ++i)
                       curve_vec[i-ind_start].setY(curve_vec[i-ind_start].y()*multiply_y);
            widget_2.addCurve(curve_vec, QColor(0,255,0));
             //Добавили зеленую линию

            for (unsigned int i=ind_start; i <= ind_finish; ++i)
            {
                 /* curve_vec[i-ind_start] = QPointF(tresolution * (i-ind_start) + curTime,
                                               (test_vec_end.at(i)-srednee_all_band)
                                                      / koeff + (currentBand + 2) * add_multiply); */
                 //Исправил 12.05.2017 - чтобы исправить тонкий сдвиг графика - на нижнюю строку:
                 curve_vec[i-ind_start] = QPointF(tresolution * (i-ind_start) + curTime_in_ms,
                                            (test_vec_end.at(i)-srednee_all_band)
                                             / koeff + (currentBand + 2) * add_multiply);
            };
            for (unsigned int i=ind_start; i <= ind_finish; ++i)
                 curve_vec[i-ind_start].setY(curve_vec[i-ind_start].y()*multiply_y);
            widget_2.addCurve(curve_vec, QColor(255,0,0));
              //Добавили красную линию

        }//for (unsigned int currentBand = 0; currentBand < bsaFile.header.nbands; currentBand++)

        PlotSettings settings;
        //settings.setAxis("X", curTime, curTime + tresolution * bsaFile.header.npoints, "UTC");
        //Подписываем ось Х внизу.
        // - неверно, секунды целые! settings.setAxis("X", curTime, curTime + tresolution * (nn_points) /*bsaFile.header.npoints*/,str_X);
        settings.setAxis("X", curTime_in_ms, curTime_in_ms + tresolution * (nn_points) /*bsaFile.header.npoints*/,str_X);



        if (otladka==1)
         {
            writeStream << "Start_UT , Start_STAR, curTime, curTime_in_ms= "
                        << UT_start.toString("yyyy:MM:dd:hh:mm:ss.zzz") << ","
                        << now.toString("yyyy:MM:dd:hh:mm:ss.zzz") << "," << curTime << "," << curTime_in_ms
                        << "\n"; //Запишет в файл параметры
            fileOut.flush();
          };//if (otladka==1)


        settings.setAxis("Y", 0.0, bsaFile.header.nbands + 2);

        //double  curTime_in_ms;
        //long curTime_in_ms_int;
        double now_star_Time_in_ms=now.toMSecsSinceEpoch()/1000.0;

        /*
        settings.setAxis("X2", now.toTime_t(), now.toTime_t() +
                         tresolution * (nn_points)  * 86164.090530833/86400.0,
                         "RA", true, Qt::darkGreen);
        */
        // !!!Посмотреть, что с приведением шкалы до звезных секунд - что-то не то! 12.05.2017

        if (otladka==1)
         {
            writeStream << "Start_STAR, now_star_Time_in_ms= "
                        << now.toString("yyyy:MM:dd:hh:mm:ss.zzz")  << now_star_Time_in_ms
                        << "\n"; //Запишет в файл параметры
            fileOut.flush();
          };//if (otladka==1)

        settings.setAxis("X2", now_star_Time_in_ms, now_star_Time_in_ms +
                         tresolution * (nn_points) /*bsaFile.header.npoints*/ *coeff_from_Tutc_to_Tstar, // * 1.20,
                         "RA", true, Qt::darkGreen);

        settings.setAxis("Y2", 0.0, bsaFile.header.nbands + 2, "", true);

        settings.autoAdjust = false;

        /* Убрал 16.05.2017
        settings.mListY2.insert(1000.0 * ( listY.lastKey() - (bsaFile.header.nbands+1) )
                        / ( calib(bsaFile.data_file[currentCanal * (bsaFile.header.nbands+1) + bsaFile.header.nbands],
                          0.0, bsaFile.header.nbands, currentCanal,otladka_add_1) - total / nn_points
                                 ),
                         "1000K ~ 100Jy");

        //
        y_metka=curve_vec[i].y();
        OutLine_1="+" + OutLine_1.setNum(scale_T) + " (10~1Jy)";
        listY_2.insert(y_metka, OutLine_1);
        //
        */


        settings.autoAdjustY = false;
        settings.mTickLabelTypeY = PlotSettings::mtList;
        settings.mListY.swap(listY);
        settings.numYTicks = bsaFile.header.nbands + 1;
        //settings.numYTicks = settings.mListY.count() + 1;

        settings.autoAdjustY2 = false;


        settings.numYTicks = settings.mListY.count() + 1;

        settings.autoAdjustY2 = false;
        //!!! -
        settings.mListY2.swap(listY_2);
          //settings.numY2Ticks = 2;//1
        settings.numY2Ticks = settings.mListY2.count() + 1; //1;

#ifdef Q_OS_WIN
        settings.MarginX = 80;
#endif

#ifdef Q_OS_LINUX
        settings.MarginX = 100;
#endif

        settings.mTickLabelTypeX = PlotSettings::mtDateTime;
        settings.mDateTimeFormatX = "hh:mm:ss";

        int star_h,star_m;
        float star_s;
        star_h=int(Impuls.Star_Time);
        star_m=int((Impuls.Star_Time-star_h)*60.0);
        star_s=(Impuls.Star_Time-star_h-star_m/60.0)*3600;


        settings.title = "Ray N " + QString::number(currentCanal+1) + ". Frequency " +
                QString::number(*bsaFile.header.fbands.begin(),'f', 3) + " - " +
                QString::number(*bsaFile.header.fbands.rbegin(),'f', 3) + " MHz.      Impuls: Ind=" +
                QString::number(Impuls.ind,'f', 0) + ", UT=" +
                Impuls.UT.toString("yyyy:MM:dd hh:mm:ss.zzz") +
                ", dT_1/2=" + QString::number(Impuls.d_T_Half_H,'f', 3) +
                ", RA="+ (star_h < 10 ? "0" : "") + QString::number(star_h) + ":"
                       + (star_m < 10 ? "0" : "") + QString::number(star_m) + ":"
                       + (star_s < 10 ? "0" : "") + QString::number(star_s,'f',3) +
                ",  Max=" + QString::number(Impuls.max_Data,'f', 3) +
                ", S/N=" + QString::number(Impuls.S_to_N,'f', 2) +
                ", Dispers=" + QString::number(Impuls.Dispers,'f', 2) +
                ", n_bands=" + QString::number(Impuls.n_bands_more_than_max,'f', 0) +
                " ;     IMP=" + Impuls.good_or_bad.c_str();

        widget_2.setPlotSettings(settings);

        if(!ui->widget->isHidden())
            widget_2.update();


        //if(ui->checkBox->isChecked())
        // сохранение в файл
        std::vector<std::string> str_vec_tmp = split(Impuls.name_file_short, '.');
        QString Name_graph_file=QString::fromStdString(str_vec_tmp.at(0))+
                "_i_" + QString::number(Impuls.ind+1000000) +
                "_r_" + QString::number(currentCanal+1)
                 + "_.png";

        Impuls.name_file_graph=Name_graph_file.toStdString();

        if (Impuls.good_or_bad=="good")
        {
        Name_graph_file=ui->leDir->text() + Name_graph_file;
               // + QString::number(Impuls.ind)  + "_.png"
        }
        else
        {
         Name_graph_file=ui->leDir->text() + "BAD/" + Name_graph_file;
        }


        if(!ui->checkBox->isChecked())
            widget_2.save(Name_graph_file,
              "PNG", 1200, 600);

        if (otladka==1)
         {
            writeStream << "END of runSaveFigure_in_File, Name_Files= "
                        << Name_graph_file << ",\n NA_OSI= " << settings.title
                        << "\n"; //Запишет в файл параметры
            fileOut.flush();
          };//if (otladka==1)


    }
    //else if(band_or_ray == 1)
}
//void grav::runSaveFigure_in_File

/*
 *
  найдем, во скольких хороших лучах произошло превышение над средним
  максимумом (теперь он в max_Data, после деления на число хороших лучей)
  Если превышение только в 1-2 лучах - это плохо, означает что мы зацепили
  мощную помеху с одного из лучей в развертке по дисперсии сквозь сдвиги по лучам.
  Пометим такой импульс как BAD , записав в импульс число сосчитанных превышений -
  мера "хорошести" импульса. Вероятно, выставить как хотя бы четверть - для 32 - восемь,
  для 6 полос - ставим 2
 */
int grav::Find_Number_good_ray_with_max_for_this_Dispers
                                   (long ind_left_Half_H, long ind_righte_Half_H,
                                    const int size_point,
                                    const unsigned int currentCanal,
                                    const unsigned int startBand,
                                    const unsigned int finishBand,
                                    float Dispers,
                                    float max_Data,
                                    float srednee_all_band,
                                    const std::vector<int> &data_file_filter
                                     //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос
                                )


{
double rezult=0;
double d_T_in_band,d_X;
double  y1;
long d_ind_x1,index_data;
int counter_good_band;

//index_data=ind_left_Half_H;
//test_vec_end[i_ind]=
//rezult=bsaFile.data_file[size_point * index_data
//        + currentCanal * (bsaFile.header.nbands+1)
//        + finishBand];



//Теперь найдем хорошие данные в нужном диапазоне для хороших полос
//- ДЛЯ ДАННОЙ ДИСПЕРСИИ и данных органичений по индексу
for (unsigned int currentBand = startBand ; currentBand <=  finishBand-1; currentBand++)
 {//Проход по полосам - ищем превышения среднего максимума в заданных пределах индексов
  //- только по хорошим полосам (БЕЗ общей полосы)

   if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)
     //Из маски =0 - не брать, =1 - брать
    {
      d_T_in_band=4.15*1000*Dispers *
         ( pow(bsaFile.header.fbands[currentBand],-2) - pow(bsaFile.header.fbands[finishBand],-2) );
      //Дисперсионная_Задержка_в_сек=4,15*1000*Dispers*(СТЕПЕНЬ(frec_min;-2) - СТЕПЕНЬ(frec_max;-2))
      //Где frec_min и frec_max - в МГц
      // Дисперсионная_Задержка_в_сек=4,15*1000*Dispers* ((pow(frec_min,-2) - (pow(frec_max,-2))
      d_X=d_T_in_band*(1000.0/bsaFile.header.tresolution);

      d_ind_x1=int(d_X+0.5);

      int found_max=0;
      rezult=0;
      for (int index_data=ind_left_Half_H; index_data<=ind_righte_Half_H; index_data++)
      {
       y1 = bsaFile.data_file.at(size_point * (index_data+d_ind_x1)
                 + currentCanal * (bsaFile.header.nbands+1)
                 + currentBand);
       rezult=rezult+y1;//-srednee_all_band;

      } ;//for (int index_data=ind_left_Half_H; index_data<=ind_righte_Half_H; index_data++)

      rezult=rezult/(ind_righte_Half_H-ind_left_Half_H+1)*1.41;
      if (rezult>=max_Data)
      {
         found_max=1;
      };
      counter_good_band=counter_good_band+found_max;

    };//if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)
     //Прибавляем все хорошие полосы

 };//for (unsigned int currentBand = finishBand-1 ; currentBand >= startBand; currentBand--)

return counter_good_band; // передаём значение счетчика в главную функцию

}
//void grav::Find_Number_good_ray_with_max_for_this_Dispers

/*
 * Функция для вычисления данных в одной точке - в зависимости от значения дисперсии
 */
double grav::Find_Data_from_Dispers(long index_data,
                                    const int size_point,
                                    const unsigned int currentCanal,
                                    const unsigned int startBand,
                                    const unsigned int finishBand,
                                    float Dispers,
                                    const std::vector<int> &data_file_filter
                                     //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос
                                )
/*
 *Цель: найти импульс(-ы ?) и его характеристики. Если импульс совсем сильный - отрисовать.
 * Видимо, лучше найти сразу все импульсы для данного луча. Значит - нужны все накопленные массивы данного луча.
 *
 */
{
    double rezult=0;
    double d_T_in_band,d_X,tmp_data;
    double x1, x2, y1, y2;
    long d_ind_x1;
    long d_ind_x2;
    float d_X_after_left_ind;

    //test_vec_end[i_ind]=
    rezult=bsaFile.data_file[size_point * index_data
            + currentCanal * (bsaFile.header.nbands+1)
            + finishBand];

    //Теперь найдем суммарный вектор в нужном диапазоне для хороших полос - ДЛЯ ДАННОЙ ДИСПЕРСИИ
    for (unsigned int currentBand = startBand ; currentBand <=  finishBand-1; currentBand++)
     {//Проход по полосам - ищем начальный суммарный вектор, с которым будем сравнивать
      //- только по хорошим полосам и БЕЗ общей полосы
      //При этом мы _спускаемся_ по полосам, начиная от самой высокочастотной.
      //Хотел спускаться по полосам, но не получилось...
       if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)
         //Из маски =0 - не брать, =1 - брать
        {
          d_T_in_band=4.15*1000*Dispers *
             ( pow(bsaFile.header.fbands[currentBand],-2) - pow(bsaFile.header.fbands[finishBand],-2) );
          //Дисперсионная_Задержка_в_сек=4,15*1000*Dispers*(СТЕПЕНЬ(frec_min;-2) - СТЕПЕНЬ(frec_max;-2))
          //Где frec_min и frec_max - в МГц
          // Дисперсионная_Задержка_в_сек=4,15*1000*Dispers* ((pow(frec_min,-2) - (pow(frec_max,-2))
          d_X=d_T_in_band*(1000.0/bsaFile.header.tresolution);

          d_ind_x1=int(d_X);
          d_ind_x2=d_ind_x1+1;
          d_X_after_left_ind=d_X-d_ind_x1;

          if (otladka==2)
            {
              writeStream << "Find_Data_from_Dispers: ray,band, ind_D, dispersion,d_T, d_X = "
                          << currentCanal << ", " << currentBand << ", "
                          << d_T_in_band << ", " << d_X << ", "
                          << " \n"; //Запишет в файл параметры
              fileOut.flush();
            };//if (otladka==1)

          /*
          for (int i_ind=0; i_ind < check_npoints; ++i_ind)
             //Проход по временнЫм точкам - присваиваем начальные значения конечного вектора в
              //интервале нужных индексов
           {

           */

         y1 = bsaFile.data_file.at(size_point * (index_data+d_ind_x1)
                     + currentCanal * (bsaFile.header.nbands+1)
                     + currentBand);

         y2 = bsaFile.data_file.at(size_point * (index_data+d_ind_x2)
                     + currentCanal * (bsaFile.header.nbands+1)
                     + currentBand);
             //calib_data_T=y1 + (y2 - y1) * (i_ind - x1) / (x2 - x1) ;

         tmp_data=y1 + (y2 - y1) * d_X_after_left_ind;
         rezult=rezult+tmp_data;
             //Прибавили интерполированное значение

             /*
           } ;//for (unsigned int i_ind=0; i_ind < check_npoints; ++i_ind)
           */

        };//if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)
         //Прибавляем все хорошие полосы

     };//for (unsigned int currentBand = finishBand-1 ; currentBand >= startBand; currentBand--)
    return rezult; // передаём значение данных в главную функцию
}

/*
 * Определяем параметры импульсов и отрисовываем их, если надо, в графические файлы - с входными параметрами:
 */
void grav::FindImpulses_in_Data(int band_or_ray, float add_tmp, double koeff, double /* start_time_in_sec */ start_time_in_sec_end,
                                double multiply_y, double scale_T, double add_multiply,
                                float srednee_all_band, float sigma_all_band,
                                float limit_sigma_all_band, float limit_sigma,
                                std::string name_file, std::string name_file_short,
                                int device, int n_bands, float resolution, /*int ray,*/ const long start_point,
                                const double mjd_start, const double star_time_in_Hour_start_file,
                                const QDateTime now, //время UT начала данных
                                const int size_point,
                                const unsigned int currentCanal,
                                const unsigned int startBand,
                                const unsigned int finishBand,
                                const int counter_good_bands,
                                const std::vector<int> &data_file_filter,
                                 //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос
                                const std::vector<float> &dispersion_maska,
                                const std::vector<float> &myltiply_vec,
                                const std::vector<float> &dispersion_vec,
                                const std::vector<float> &test_vec, std::vector<float> &test_vec_end,
                                const std::vector<float> &test_vec_end_tmp,
                                std::vector<Point_Impuls_D> &Impulses_vec,
                                int cod_otladka)
/*
 *Цель: найти импульс(-ы ?) и его характеристики. Если импульс совсем сильный - отрисовать.
 * Видимо, лучше найти сразу все импульсы для данного луча. Значит - нужны все накопленные массивы данного луча.
 *
 */
{
    //int ind;
    long counter_otladka=0;
    long cod_otladka_tmp=cod_otladka;
    double MJD;
    QDateTime UT;
    float Star_Time, d_T_zona, d_T_Half_H, max_Data, S_to_N, Dispers;
    //int n_similar_Impuls_in_rays=1;
     //Кусок параметров импульса Point_Impuls_D , которые мы будем определять внутри данной функции FindImpulses_in_Data
    //double star_time_in_Hour_Impuls;
    //QDateTime now_Impuls;
    QString OutLine_1;
    QErrorMessage errorMessage;
    //Запись в конец файла:
    std::ofstream log_out;                    //создаем поток для ДО-записи лога ошибок

    Point_Impuls_D Current_Impuls;


    if ((otladka==1)  && (cod_otladka>=292))
       {
    log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
    log_out  << "Voshel v: FindImpulses_in_Data\n" ;
    // сама запись
    log_out  << bsaFile.name_file_of_datas.toStdString()
             << " : PARAMETERS:  ===\n";
    log_out  << " counter_otladka=" << counter_otladka;
    log_out  << " add_tmp=" << add_tmp;
    log_out  << ", start_time_in_sec=" << start_time_in_sec_end
             << " ; multiply_y=" << multiply_y  << "\n";      // сама запись
    log_out  << ", scale_T=" << scale_T;
    log_out  << ", add_multiply=" << add_multiply;
    log_out  << ", srednee_all_band=" << srednee_all_band;
    log_out  << ", sigma_all_band=" << sigma_all_band;
    log_out  << ", limit_sigma_all_band=" << limit_sigma_all_band;
    log_out  << ", limit_sigmaT=" << limit_sigma;
    log_out  << ", name_file=" << name_file  << ",";
    log_out  << ", name_file_short=" << name_file_short << ",";
    log_out  << ", device=" << device;
    log_out  << ", n_bands=" << n_bands;
    log_out  << ", resolution=" << resolution;
    log_out  << ", start_point=" << start_point;
    log_out  << ", mjd_start=" << mjd_start;


    log_out  << ", star_time_in_Hour_start_file=" << star_time_in_Hour_start_file;
    QString str_tmp=now.toString("yyyy.MM.dd hh:mm:ss.zzz");
    log_out  << ", now=" << str_tmp.toStdString();
    log_out  << ", size_point=" << size_point;
    log_out  << ", start_point=" << start_point;
    log_out  << ", currentCanal=" << currentCanal;
    log_out  << ", startBand=" << startBand;
    log_out  << ", finishBand=" << finishBand;
    log_out  << ", counter_good_bands=" << counter_good_bands;
    log_out  << " \n\n";
                   /* */

    log_out.close();      // закрываем файл
    };




    std::vector<float> work_vec(test_vec_end_tmp.size());
    //Рабочий вектор, сюда будем класть текущие найденные значения (для разных дисперсий и т.п.)
    float step_dispers, dispers_work;
    int ind_dispers_maska;

    float maximum, level_max;
    maximum=-1000000;
    level_max=sigma_all_band*limit_sigma_all_band;

    if (otladka==1)
      {
         writeStream << "\n Start of FindImpulses_in_Data , input parameters= \n "
                     //<< "UT= " << Current_Impuls.UT << " , "
                     << "band_or_ray= " << band_or_ray << " , "
                     << "add_tmp, koeff, multiply_y, scale_T, add_multiply =" << add_tmp << " , "  << koeff << " , "
                     << multiply_y << " , "  << scale_T << " , " << add_multiply << "\n"
                     << "currentCanal= " << currentCanal << " , "
                     << "startBand= " <<  startBand << " , "
                     << "finishBand= " << finishBand << " , "
                     << "counter_good_bands= " << counter_good_bands << " , "
                     << "resolution= " << resolution << " , "
                     << "start_point= " << start_point << " , "
                     << "star_time_in_Hour_start_file= " << star_time_in_Hour_start_file << " , "
                     << "\n"; //Запишет в файл параметры

         /*
         double start_time_in_sec,

                                         float srednee_all_band, float sigma_all_band,
                                         float limit_sigma_all_band, float limit_sigma,
                                         std::string name_file, std::string name_file_short,
                                         int device, int n_bands,
                                         const double mjd_start,  const QDateTime now,
                                         const int size_point,
                                         */

         fileOut.flush();
      };//if (otladka==1)



        if ((otladka==1)  && (cod_otladka>=292))
       {
    log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
    log_out  << "Voshli v  FindImpulses_in_Data, DO perebora i ot -60 do +60 i DO parametra Dispers";
    log_out  << " counter_otladka=" << counter_otladka << ", cod_otladka=" << cod_otladka;
    log_out  << " test_vec_end_tmp.size()=" << test_vec_end_tmp.size() ;
    log_out  << " \n";
                   /* */
    log_out.close();      // закрываем файл
    };



    for (unsigned int i=0;i<test_vec_end_tmp.size();++i)
      //Пробегаем по вектору, находим самое максимальное значение
        if ((test_vec_end_tmp.at(i)>=level_max) && (maximum<test_vec_end_tmp.at(i)))
            maximum=test_vec_end_tmp.at(i);

    if (maximum<0) //нет ни одного приличного максимума
        return ;

    long ind_max,ind_left,ind_righte,ind_left_Half_H,ind_righte_Half_H;
    ind_max=0;

    for (unsigned int i=0;i<test_vec_end_tmp.size();++i)
      {
      //Копируем базовый конечный вектор во временный - с ним и будем работать
         test_vec_end.at(i)=test_vec_end_tmp.at(i);
         //Обнуляем рабочий вектор
         work_vec.at(i)=0;
      };

    while (ind_max>=0)
     {
      maximum=-1000000;
      for (unsigned int i=0;i<test_vec_end.size();++i)
       {//Пробегаем по вектору, находим самое максимальное значение
        if ((test_vec_end.at(i)>=level_max) && (maximum<test_vec_end.at(i)))
          {
            maximum=test_vec_end.at(i);
            ind_max=i;
          };
       };//for (unsigned int i=0;i<test_vec_end_tmp.size();++i)

      if (maximum<0)
       {//Ничего не нашли, заканчиваем
         ind_max=-1;
       }
      else
       {//Находим основные параметры импульса - границы и пр.
         ind_left=ind_max;
         ind_righte=ind_max;
         /*
         while ((ind_left>0) && (test_vec_end.at(ind_left-1)>0)
                && (test_vec_end.at(ind_left-1)<test_vec_end.at(ind_left)))
           {
             ind_left=ind_left-1;
           };
         while ((ind_righte<test_vec_end.size()-1) && (test_vec_end.at(ind_righte+1)>0)
                && (test_vec_end.at(ind_righte+1)<test_vec_end.at(ind_righte)))
           {
             ind_righte=ind_righte+1;
           };
         */
         //Закоментировали: Слишком много вторичных максимумов!!

         while ((ind_left>0) && (test_vec_end.at(ind_left-1)>0)
                && ((test_vec_end.at(ind_left-1)-srednee_all_band)>level_max ))
           {
             ind_left=ind_left-1;
           };
         while ((ind_righte<test_vec_end.size()-1) && (test_vec_end.at(ind_righte+1)>0)
                && ((test_vec_end.at(ind_righte+1)-srednee_all_band)>level_max ))
           {
             ind_righte=ind_righte+1;
           };

         //Нашли крайние левую и правую границы, до которых идет спадение от максимального значения
         //(а далее - уже новый рост или граница массива)
         //ПО НОВОМУ: до края необходимого превышения.
         d_T_zona=(ind_righte-ind_left)*resolution/1000.0;
         //нашли зону распространения импульса (грубо)

         if ((otladka==1)  && (cod_otladka==293))
        {
     log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
     log_out  << "Voshli v  FindImpulses_in_Data, PROSTO TEST 11111 POSLE nachal.param: ";
     log_out  << " cod_otladka=" << cod_otladka;
     log_out  << ", otladka=" << otladka;
     log_out  << " test_vec_end.size()=" << test_vec_end.size();
     log_out  << " ind_max=" << ind_max << " ind_left=" << ind_left
              << " ind_righte=" << ind_righte
              << " ,test_vec_end.at(ind_max)=" << test_vec_end.at(ind_max)
              << " ,level_max="  <<level_max;
     log_out  << " \n";
                    /* */
     log_out.close();      // закрываем файл

     log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
     log_out  << " , test_vec_end.at(ind_righte)=" << test_vec_end.at(ind_righte);
     log_out  << " \n";
     log_out.close();      // закрываем файл
     };

         if (otladka==1)
           {
              writeStream << "\n Find_zona: ind_left,ind_righte, ind_max, d_T_zona, level_max = "
                          << ind_left << " , "
                          << ind_righte << " , "
                          << ind_max << " , "
                          << d_T_zona << " , " << level_max
                          << "\n"; //Запишет в файл параметры
              fileOut.flush();
           };//if (otladka==1)

         //Далее - "выжигаем" нулями эти окрестности (найденного текущего наибольшего максимума),
         //чтобы потом начать поиск оставшихся максимумов (с меньшими значениями
         //- пока они не опустятся ниже ищумого порога)
         for (unsigned int i=ind_left;i<=ind_righte;++i)
          {
           if (otladka==1)
               {
                  writeStream << "BYLO v VECTORE i,test_vec_end.at(i) = "
                              << i << " , "
                              << test_vec_end.at(i)
                              << "\n"; //Запишет в файл параметры
                  fileOut.flush();
               };//if (otladka==1)

           test_vec_end.at(i)=0;
          }


         Dispers=dispersion_vec.at(ind_max);
         // Осталось найти: а) ширину по половине максимума (над средним уровнем)
         //б) более лучшее значение дисперсии - перебором более мелкой сетки

         //Делаем а): ищем ширину по уровню половины максимума (превышения над средним уровнем).
         //Нужна функция вычисления значения test_vec_end.at(i) в точке с индексом i
         work_vec.at(ind_max)=Find_Data_from_Dispers(ind_max,size_point,
                                             currentCanal,startBand,finishBand,Dispers,
                                             data_file_filter
                                              //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос
                                         );

         if ((otladka==1)  && (cod_otladka==293))
        {
     log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
     log_out  << "Proshli Find_Data_from_Dispers, ind_max=" << ind_max
              << ", work_vec.at(ind_max)=" << work_vec.at(ind_max);
     log_out  << " \n";
     log_out.close();      // закрываем файл
     };
         double rezult_Half_H=(work_vec.at(ind_max)-srednee_all_band)/2.0;
            //Половина от максимального значения над медианным уровнем
         ind_left_Half_H=ind_max;
         ind_righte_Half_H=ind_max;
         work_vec.at(ind_left_Half_H)=Find_Data_from_Dispers(ind_left_Half_H,size_point,
                                             currentCanal,startBand,finishBand,Dispers,
                                             data_file_filter);
         work_vec.at(ind_righte_Half_H)=Find_Data_from_Dispers(ind_righte_Half_H,size_point,
                                             currentCanal,startBand,finishBand,Dispers,
                                             data_file_filter);
         while ((ind_left_Half_H>0) && ((work_vec.at(ind_max)-work_vec.at(ind_left_Half_H))<rezult_Half_H) )
             //??? Почему знак МЕНЬШЕ??? 14.09.2017 ОТВЕТ: потому что идем от максимума. Все верно.
           {
             ind_left_Half_H=ind_left_Half_H-1;
             work_vec.at(ind_left_Half_H)=Find_Data_from_Dispers(ind_left_Half_H,size_point,
                                                 currentCanal,startBand,finishBand,Dispers,
                                                 data_file_filter);
             //rezult=work_vec.at(ind_left_Half_H);
           };
         while ((ind_righte_Half_H<test_vec_end.size()-1) &&
                ((work_vec.at(ind_max)-work_vec.at(ind_righte_Half_H))<rezult_Half_H) )
             //??? Почему знак МЕНЬШЕ??? 14.09.2017 ОТВЕТ: потому что идем от максимума. Все верно.
           {
             ind_righte_Half_H=ind_righte_Half_H+1;
             work_vec.at(ind_righte_Half_H)=Find_Data_from_Dispers(ind_righte_Half_H,size_point,
                                                 currentCanal,startBand,finishBand,Dispers,
                                                 data_file_filter);
           };
         //Нашли крайние левую и правую границы, до которых идет спадение от максимального значения
         //до 1/2 величины превышения максимума над средним уровнем

         if ((otladka==2) )
          {
     log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
     log_out  << "Posle while ((ind_left_Half_H>=0) i while ((ind_righte_Half_H<=test_vec_end.size()-1) ";
     log_out  << " \n";
     log_out.close();      // закрываем файл
          };

         if (otladka==1)
           {
              writeStream << "Find_zona: ind_left_Half_H,ind_righte_Half_H, ind_max, rezult_Half_H = "
                          << ind_left_Half_H << " , "
                          << ind_righte_Half_H << " , "
                          << ind_max << " , "
                          << rezult_Half_H
                          << "\n"; //Запишет в файл параметры
              fileOut.flush();
           };//if (otladka==1)

         for (unsigned int i=0;i<100;++i)
           {
           //Перебираем дисперсии в массиве-маске и выбираем наиболее близкий индекс
             if (abs(Dispers-dispersion_maska.at(i))<0.1)
                {ind_dispers_maska=i;};
           };

         if (ind_dispers_maska>0)
         {
          step_dispers=0.05*(dispersion_maska.at(ind_dispers_maska)-dispersion_maska.at(ind_dispers_maska-1));
          if (step_dispers>1.0)
            {step_dispers=1.0;};
         }
         else
         {
          step_dispers=0.1;
         };

         double summ_current=-100000.0;
         double summ_max=-100000.0;
         double max_current=-100000.0;
         long ind_max_current, ind_max_best;
         float Dispers_best=Dispers;

         if (/*(otladka==1)  && */ (cod_otladka_tmp>=292))
        {
     log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
     log_out  << "Voshli v  FindImpulses_in_Data, DO perebora i ot -60 do +60 i POSLE nachal.param: ";
     log_out  << " Dispers=" << Dispers << ", cod_otladka=" << cod_otladka;
     log_out  << ", otladka=" << otladka;
     log_out  << " test_vec_end_tmp.size()=" << test_vec_end_tmp.size()
               << "  ind_max=" << ind_max
              << ", ind_left_Half_H=" << ind_left_Half_H
                 << ", ind_righte_Half_H=" << ind_righte_Half_H;

     log_out  << " \n";
                    /* */
     log_out.close();      // закрываем файл
     };


         if ((otladka==1)  && (cod_otladka==293))
        {
     log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
     log_out  << "Voshli v  FindImpulses_in_Data, PROSTO TEST 2222 POSLE nachal.param: ";
     log_out  << " Dispers=" << Dispers << ", cod_otladka=" << cod_otladka;
     log_out  << ", otladka=" << otladka;
     log_out  << " test_vec_end_tmp.size()=" << test_vec_end_tmp.size()
               << "  ind_max=" << ind_max
              << ", ind_left_Half_H=" << ind_left_Half_H
                 << ", ind_righte_Half_H=" << ind_righte_Half_H;

     log_out  << " \n";
                    /* */
     log_out.close();      // закрываем файл
     };
         //Делаем б) более лучшее значение дисперсии - перебором более мелкой сетки
         for (int i=-60;i<=60;++i)
           {
             dispers_work=Dispers+step_dispers*i;
             if (dispers_work<1)
                dispers_work=1.0;
             if (dispers_work>2000)
                dispers_work=2000.0;
             //Далее ищем в области ширины по половине максимума
             summ_current=0;
             max_current=-100000.0;
             for (int index=ind_left_Half_H;index<=ind_righte_Half_H;++index)
               {
                 work_vec.at(index)=Find_Data_from_Dispers(index,size_point,
                                                     currentCanal,startBand,finishBand,dispers_work,
                                                     data_file_filter);
                 summ_current=summ_current+(work_vec.at(index)-srednee_all_band);
                 if (work_vec.at(index)>max_current)
                   {
                    max_current=work_vec.at(index);
                    ind_max_current=index;
                   };
               }; //for (int index=ind_left_Half_H;index<=ind_righte_Half_H;++index)
             if (summ_current>summ_max)
               {
                 summ_max=summ_current;
                 ind_max_best=ind_max_current;
                 Dispers_best=dispers_work;

                 if (otladka==1)
                   {
                      writeStream << "Find_zona: summ_max,ind_max_best, Dispers_best = "
                                  << summ_max << " , "
                                  << ind_max_best << " , "
                                  << Dispers_best
                                  << "\n"; //Запишет в файл параметры
                      fileOut.flush();
                   };//if (otladka==1)
               };

             counter_otladka=counter_otladka+1;
             if ((otladka==1)  && (cod_otladka==293))
                {
             log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
             log_out  << "Vnutri FindImpulses_in_Data, perebor i ot -60 do +60,  i===" << i;
             log_out  << " counter_otladka=" << counter_otladka;
             log_out  << " \n";
                            /* */
             log_out.close();      // закрываем файл
             };


           }; //for (int i=-10;i<=+10;++i)

         //ПОВТОРНО ПЕРЕ-определяем границы по уровню 1/2
         ind_max=ind_max_best;
         Dispers=Dispers_best;
         work_vec.at(ind_max)=Find_Data_from_Dispers(ind_max,size_point,
                                             currentCanal,startBand,finishBand,Dispers,
                                             data_file_filter
                                              //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос
                                         );

         rezult_Half_H=(work_vec.at(ind_max)-srednee_all_band)/2.0;
         ind_left_Half_H=ind_max;
         ind_righte_Half_H=ind_max;
         work_vec.at(ind_left_Half_H)=Find_Data_from_Dispers(ind_left_Half_H,size_point,
                                             currentCanal,startBand,finishBand,Dispers,
                                             data_file_filter);
         work_vec.at(ind_righte_Half_H)=Find_Data_from_Dispers(ind_righte_Half_H,size_point,
                                             currentCanal,startBand,finishBand,Dispers,
                                             data_file_filter);
         while ((ind_left_Half_H>0)
                && ((work_vec.at(ind_max)-work_vec.at(ind_left_Half_H))<rezult_Half_H) )
           {
             ind_left_Half_H=ind_left_Half_H-1;
             work_vec.at(ind_left_Half_H)=Find_Data_from_Dispers(ind_left_Half_H,size_point,
                                                 currentCanal,startBand,finishBand,Dispers,
                                                 data_file_filter);
             //rezult=work_vec.at(ind_left_Half_H);
           };
         while ((ind_righte_Half_H<test_vec_end.size()-1)
                && ((work_vec.at(ind_max)-work_vec.at(ind_righte_Half_H))<rezult_Half_H) )
           {
             ind_righte_Half_H=ind_righte_Half_H+1;
             work_vec.at(ind_righte_Half_H)=Find_Data_from_Dispers(ind_righte_Half_H,size_point,
                                                 currentCanal,startBand,finishBand,Dispers,
                                                 data_file_filter);
           };
         //Нашли ОКОНЧАТЕЛЬНЫЕ крайние левую и правую границы, до которых идет спадение от максимального значения
         //до 1/2 величины превышения максимума над средним уровнем

         d_T_Half_H=(ind_righte_Half_H-ind_left_Half_H)*resolution/1000.0;
         //нашли зону распространения импульса (точно, по уровню 1/2)
         max_Data=(work_vec.at(ind_max)-srednee_all_band);
         S_to_N=max_Data/sigma_all_band;
         max_Data=max_Data/counter_good_bands;

         //Теперь найдем, во скольких хороших лучах произошло превышение над средним
         //максимумом (теперь он в max_Data, после деления на число хороших лучей)
         //Если превышение только в 1-2 лучах - это плохо, означает что мы зацепили
         //мощную помеху с одного из лучей в развертке по дисперсии сквозь сдвиги по лучам.
         //Пометим такой импульс как BAD , записав в импульс число сосчитанных превышений -
         //мера "хорошести" импульса. Вероятно, выставить как хотя бы четверть - для 32 - восемь,
         //для 6 полос - ставим 2
         int n_rays_more_than_max=0;
         std::string good_or_bad="good";
         //currentCanal=

         //std::vector<int> data_file_filter_tmp(countsCanal*(bsaFile.header.nbands+1));
         std::vector<int> data_file_filter_tmp(data_file_filter.size());
         data_file_filter_tmp=data_file_filter;
         //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос;
          //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос

         if ((otladka==1)  && (cod_otladka==293))
            {
         counter_otladka=counter_otladka+1;
         log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
         log_out  << "Nashli parametry, pered Find_Number_good_ray_with_max_for_this_Dispers";
         log_out  << " counter_otladka=" << counter_otladka;
         log_out  << " \n";
                        /* */
         log_out.close();      // закрываем файл
         };
         n_rays_more_than_max=Find_Number_good_ray_with_max_for_this_Dispers(ind_left_Half_H,ind_righte_Half_H,
                                                                             size_point,
                                                                             currentCanal,startBand,finishBand,Dispers,
                                                                             max_Data,srednee_all_band,
                                                                             data_file_filter);


//!!!!!!!!!!!!!! Здесь используют число, которое надо бы потом вводить через файл установок:
         if (n_rays_more_than_max>=(counter_good_bands/3.0+1))
         {
             good_or_bad="good";
         }
         else
         {
             good_or_bad="BAD";
         };

         UT=now;
         UT = UT.addMSecs((ind_max)*resolution);
           //Неверно!!! не поделили на 1000  НЕТ, ВЕРНО! ПОСКОЛЬКУ РАБОТАЕМ С МИЛЛИСЕКУНДАМИ!

                   // now = now.addMSecs((int)(start_time_in_sec*1000));
         Star_Time=star_time_in_Hour_start_file+((start_point+ind_max)*resolution/1000.0)/3600*coeff_from_Tutc_to_Tstar;
         MJD=mjd_start + ((start_point+ind_max)*resolution/1000.0) / 86400;


         std::string name_file_graph="";
         Current_Impuls={name_file,name_file_short,name_file_graph,good_or_bad,
                   //имя файла данных с директорией, имя только самого файла,
                   //имя графического файла (без директории), хороший или плохой
                 device,n_bands,resolution,currentCanal,ind_max+start_point,
                 MJD,UT,
                 Star_Time, d_T_zona, d_T_Half_H, max_Data, S_to_N, Dispers,
                 1,0,n_rays_more_than_max};

         if (otladka==2)
                {
         OutLine_1="Импульс== "
                     + QString::number(Current_Impuls.d_T_zona,'f',3) + ";"
                     + QString::number(Current_Impuls.d_T_Half_H,'f',3) + "; Max="
                     + QString::number(Current_Impuls.max_Data,'f',3) + " S/N="
                     + QString::number(Current_Impuls.S_to_N,'f',3) + "; DM="
                     + QString::number(Current_Impuls.Dispers,'f',1) + "; n_rays="
                     + QString::number(Current_Impuls.n_bands_more_than_max,'f',0);
             ui->LineEdit->setText(OutLine_1);
             //errorMessage.showMessage(OutLine_1);
             //errorMessage.exec();
             };


         // Добавление элемента в конец вектора
         Impulses_vec.push_back(Current_Impuls);

         //Если импульс силен, создаем рисунок с ним:

         if (otladka==1)
           {

             /*Impuls.UT.toString("yyyy:MM:dd hh:mm:ss.zzz") +
             ", dT_1/2=" + QString::number(Impuls.d_T_Half_H,'f', 3) +
             ",  Max=" + QString::number(Impuls.max_Data,'f', 3) +
               */
              writeStream << "\n Pered runSaveFigure_in_File   Current_Impuls= \n "
                          << "UT= " << Current_Impuls.UT.toString("yyyy:MM:dd hh:mm:ss.zzz") << " , "
                          << "d_T_zona= " << Current_Impuls.d_T_zona << " , "
                          << "d_T_Half_H= " << Current_Impuls.d_T_Half_H << " , "
                          << "resolution= " << Current_Impuls.resolution << " , "
                          << "Star_Time= " << Current_Impuls.Star_Time << " , "
                          << "ray= " << Current_Impuls.ray << " , "
                          << "ind_max= " << ind_max << " , "
                          << "ind= " << Current_Impuls.ind << " , "
                          << "max_Data= " << Current_Impuls.max_Data << " , "
                          << "S_to_N= " << Current_Impuls.S_to_N << " , "
                          << "Dispers= " << Current_Impuls.Dispers << " , "
                          << "\n"; //Запишет в файл параметры
              fileOut.flush();
           };//if (otladka==1)
         runSaveFigure_in_File(band_or_ray,add_tmp,koeff,
                               /*now,*/ /*start_time_in_sec */ start_time_in_sec_end,start_point,
                               multiply_y,scale_T,add_multiply,srednee_all_band,
                               test_vec,//Начальные сложенные данные , без учета дисперсий
                               work_vec,//Конечные сложенные данные , часть вектора - для лучшей дисперсии
                               test_vec_end_tmp,//Конечные сложенные данные , накопленные максимальные по разным дисперсиям
                               Current_Impuls);
         Impulses_vec.at(Impulses_vec.size()-1).name_file_graph=Current_Impuls.name_file_graph;


       };// if (maximum<0)  else
     };//while (ind_max<0)


}


/*
 * Проходим по массиву импульсов, ищем одни и те же на разных лучах
 * (то есть число родственных импульсов поперек лучей - если во всех это помехи):
 */
void grav::Analyze_and_rewrite_ArrayImpulses(unsigned int countsCanal, //Число лучей в диаграмме
                                             std::vector<Point_Impuls_D> &Impulses_vec)
/*
 *Цель: найти родственные импульсы и пометить их, изменив характеристики.
 * Имеем ввиду, что такие импульсы, если они все поперек МНОГИХ лучей - чаще всего - помехи
 * (исключая очень редкого случая сильного космического импульса далеко в стороне)
 *
 */
{
    int memory_otladka=otladka;
    otladka=1;
    int count_impuls=Impulses_vec.size();
    Point_Impuls_D Current_Impuls;
    std::ofstream log_out;    //создаем поток для до-записи в файл
    //unsigned int countsCanal; //Число лучей в диаграмме
    int maska_rays[countsCanal];
    for (int i=0;i<countsCanal;i++)
     {
       maska_rays[i]=0;
     };

    if (count_impuls>0)
      {
        for (int i=0;i<count_impuls;++i)
         {
            Impulses_vec.at(i).blended=0;
            Current_Impuls=Impulses_vec.at(i);
            int counter_rays=0;
            int ind_start=Current_Impuls.ind-int(2.0*Current_Impuls.d_T_Half_H*1000/Current_Impuls.resolution + 0.5);
            int ind_finish=Current_Impuls.ind+int(2.0*Current_Impuls.d_T_Half_H*1000/Current_Impuls.resolution + 0.5);
            for (int i=0;i<countsCanal;i++)
             {
               maska_rays[i]=0;
             };

            if (otladka==1)
             {
                writeStream << "Work with blends: ind,ind_start,ind_finish,ray_current="
                            << Current_Impuls.ind << ",  "
                            << ind_start << "," << ind_finish << ","
                            <<  Current_Impuls.ray << "\n"; //Запишет в файл индекс
                fileOut.flush();
              };//if (otladka==1)

            for (int j=0;j<count_impuls;++j)
             {
               if ((Impulses_vec.at(j).ind>=ind_start) && (Impulses_vec.at(j).ind<=ind_finish))
               {
                 if (maska_rays[Impulses_vec.at(j).ray]==0)
                  {
                   counter_rays=counter_rays+1;
                   maska_rays[Impulses_vec.at(j).ray]=1;
                  };
                 if (otladka==1)
                  {
                     /* writeStream << "FOUND nearly for: j,j_ind,j_ray,counter_rays=" << j << ",  "
                                 << Impulses_vec.at(j).ind << ","
                                 << Impulses_vec.at(j).ray << ","
                                 << counter_rays << "\n"; //Запишет в файл текущие параметры
                     fileOut.flush();
                     */

                     log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
                     log_out  << "  in impuls IN LIMITS ind=: " ;
                     // сама запись
                     log_out << "   ind_start=" << ind_start << ", ind_finish=" << ind_finish
                             << " , j (imp)=" << j
                             << ", ind=" << Impulses_vec.at(j).ind
                             << ", ray_j=" << Impulses_vec.at(j).ray
                             << ", maska_ray_j=" << maska_rays[Impulses_vec.at(j).ray]
                             << ", Impulses_vec.at(j).ray=" << Impulses_vec.at(j).ray
                             << ", counter_rays=" << counter_rays
                             << "\n";

                     log_out.close();      // закрываем файл



                   };//if (otladka==1)
                 if ((Impulses_vec.at(j).ray==Current_Impuls.ray) && (Impulses_vec.at(j).ind!=Current_Impuls.ind))
                  {
                   Current_Impuls.blended=Current_Impuls.blended+1;
                   //counter_rays=counter_rays-1;
                   //БЫЛО: чтобы не увеличивать число лучей на бленде, но после применения maska_rays[] - не нужно
                   if (otladka==2)
                    {
                       writeStream << "FOUND BLEND for: j,j_ind,j_ray,,counter_rays,BLENDS=" << j << ",  "
                                   << Impulses_vec.at(j).ind << ","
                                   << Impulses_vec.at(j).ray << ","
                                   << counter_rays
                                   << "," << Current_Impuls.blended << "\n"; //Запишет в файл текущие параметры
                       fileOut.flush();
                     };//if (otladka==1)
                  }
               };//if ((Impulses_vec.at(j).ind>=ind_start) && (Impulses_vec.at(j).ind<=ind_finish))
             };//for (int j=0;j<count_impuls;++j)
            Current_Impuls.n_similar_Impuls_in_rays=counter_rays;
            Impulses_vec.at(i)=Current_Impuls;
             //поправили данные для конкретного импульса - на предмет соседних импульсов
         };//for (int i=0;i<count_impuls;++i)
      };//if (count_impuls>0)

    otladka=memory_otladka;

    //И переместить файлы! MoveFile (path1, path2);
      //https://ru.stackoverflow.com/questions/62276/%D0%9F%D0%B5%D1%80%D0%B5%D0%BD%D0%BE%D1%81-%D1%84%D0%B0%D0%B9%D0%BB%D0%B0-%D0%B2-%D0%A1
    //http://www.prog.org.ru/topic_15773_0.html     bool QFile::rename ( const QString & newName )
//http://www.codenet.ru/progr/cpp/sprd/rename.php - ЭТО?


} //void grav::Analyze_and_rewrite_ArrayImpulses(...


void grav::Write_ArrayImpulses_in_file(std::string out_data_dir,
                                       int device, int countsCanal, int n_bands,
                                       float resolution,
                                       const QString date_begin, // Дата начала регистрации файла в UTC, считано ранее из заголовка
                                       const QString time_begin, // Время начала регистрации файла в UTC, считано ранее из заголовка
                                       const float step_in_star_second,//шаг в звездных секундах
                                       const double star_time_in_Hour_start_file,//стартовая ЗВЕЗДНАЯ секунда НАЧАЛА часа (файла)
                                       const double star_time_start_end_H,//стартовая ЗВЕЗДНАЯ секунда часа, с которой все начинается
                                       const double start_time_in_sec_end,//стартовая ВРЕМЕННАЯ секунда часа, с которой все начинается
                                       const long start_point,
                                       const double mjd_start,
                                       const int counter_median_point_in_deriv,//точек в массивах
                                       const int i, //Номер считанного КУСКА-порции файла данных по counter_median_point_in_deriv
                                       double deriv_star,
                                            // количество сдвиговых секунд,вычисленных из ЗВЕЗДНЫХ секунд в порции данных
                                            //- реально их должно быть меньше, чтобы не было разрывов в данных
                                       std::vector<Point_Impuls_D> &Impulses_vec)
/*
 * Записываем массив импульсов в файл, по одному импульсу на строку.
 * В дальнейшем считаем эти импульсы в базу.
 *
 */
{
    int count_impuls=Impulses_vec.size();
    if (count_impuls>0)
      {  //Пишем файл импульсов

        QString date_and_time=date_begin + " " + time_begin;
        QDateTime start_time_of_file = QDateTime::fromString(date_and_time, "dd.MM.yyyy HH:mm:ss");
        start_time_of_file.setTimeSpec(Qt::UTC);
                   //Определили UTC старта файла
        QDateTime start_time_of_data = start_time_of_file;
        //double d_t_in_sec=((star_time_start_end_H-star_time_in_Hour_start_file)*3600.0 /*+step_in_star_second/2 */ )
        //          /coeff_from_Tutc_to_Tstar;

        double d_t_in_sec=((star_time_start_end_H-star_time_in_Hour_start_file)*3600.0
                               +i*deriv_star)
                     /coeff_from_Tutc_to_Tstar;


        start_time_of_data = start_time_of_data.addMSecs(d_t_in_sec*1000);
        double mjd_start_inside=mjd_start+d_t_in_sec/86400.0;
        //double star_time_start_end_H_inside=star_time_start_end_H;
        //        //+(step_in_star_second/2.0)/3600.0;

        double star_time_start_end_H_inside=star_time_start_end_H+
                    +(i*deriv_star*coeff_from_Tutc_to_Tstar)/3600.0;
             //Сдвинули на середину диапазона звездное время

        QDateTime start_time_of_data_tmp = start_time_of_data;
        QString str_name_file, Qstr_tmp;
        Qstr_tmp=out_data_dir.c_str();
        int star_h,star_m,star_s;
        double mjd_start_inside_tmp=mjd_start_inside;
        double star_time_tmp=star_time_start_end_H_inside;

        int count_impuls=Impulses_vec.size();
        Point_Impuls_D Current_Impuls;

        int cur_median_point = 0;

        //for (int cur_median_point = 0; cur_median_point < counter_median_point_in_deriv; cur_median_point++)
          {
           star_time_tmp=star_time_start_end_H_inside+(step_in_star_second*cur_median_point/3600.0);
           star_h=int(star_time_tmp);
           star_m=int((star_time_tmp-star_h)*60.0);
           star_s=int((star_time_tmp-star_h-star_m/60.0)*3600 +0.5);
           str_name_file=Qstr_tmp + "imp_" + (star_h < 10 ? "0" : "") + QString::number(star_h)
                + (star_m < 10 ? "0" : "") + QString::number(star_m)
                + (star_s < 10 ? "0" : "") + QString::number(star_s)
                + "_N"+ QString::number(device) +
                + "_B"+ QString::number(n_bands) +
                + "_" + start_time_of_data_tmp.toString("yyyyMMddhhmmss")+".txt";

           d_t_in_sec=step_in_star_second*cur_median_point/coeff_from_Tutc_to_Tstar;
           start_time_of_data_tmp = start_time_of_data;
           start_time_of_data_tmp = start_time_of_data_tmp.addMSecs(d_t_in_sec*1000);
           mjd_start_inside_tmp=mjd_start_inside+d_t_in_sec/86400.0;


           QFile fileOut_IMP_data(str_name_file);
            // Связываем объект с файлом fileout.txt
           QTextStream writeStream(&fileOut_IMP_data); // Создаем объект класса QTextStream
            // и передаем ему адрес объекта fileOut
           QString str1,str_tmp;


           if(fileOut_IMP_data.open(QIODevice::WriteOnly | QIODevice::Text))
              { // Если файл успешно открыт для записи в текстовом режиме
                writeStream.setCodec(QTextCodec::codecForName("cp1251"));

                //str_tmp = QString("%1").arg(mjd_start_inside_tmp, 0, 'f', 8);

                str1 = "UT=" + start_time_of_data_tmp.toString("yyyy-MM-dd hh:mm:ss.zzz") +
                       "|MJD=" + QString("%1").arg(mjd_start_inside_tmp, 0, 'f', 8) +
                       "|RA=" + QString("%1").arg(star_time_tmp, 0, 'f', 4);
                //str1 = date.toString("dd.MM.yyyy ")+time.toString("hh:mm:ss.zzz");


                for (int i=0;i<count_impuls;++i)
                //for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)
                 //Проход по лучам БСА - пишем среднемедианные и прочие значения всех лучей
                  {
                    Current_Impuls=Impulses_vec.at(i);

                    str1 = Current_Impuls.name_file_short.c_str() ; //+ " ;" + Current_Impuls.name_file_graph.c_str() ;
                    str1 = str1 + ";" + Current_Impuls.name_file_graph.c_str() + ";" +
                            QString::number(Current_Impuls.device,'f', 0) + ";" +
                            QString::number(Current_Impuls.n_bands,'f', 0) + ";" +
                            QString::number(Current_Impuls.ray,'f', 0) + ";" +
                            QString::number(Current_Impuls.ind,'f', 0) + ";" +
                            QString::number(Current_Impuls.MJD,'f', 8) + ";" +
                            Current_Impuls.UT.toString("yyyy:MM:dd hh:mm:ss.zzz") +
                            ";" + QString::number(Current_Impuls.Star_Time,'f', 6) +
                            ";" + QString::number(Current_Impuls.d_T_zona,'f', 3) +
                            //d_T_zona=(ind_righte-ind_left)*resolution/1000.0;
                            //нашли зону распространения импульса (грубо) в секундах
                            ";" + QString::number(Current_Impuls.d_T_Half_H,'f', 3) +
                            ";" + QString::number(Current_Impuls.max_Data,'f', 3) +
                            ";" + QString::number(Current_Impuls.S_to_N,'f', 2) +
                            ";" + QString::number(Current_Impuls.Dispers,'f', 2) +
                            ";" + QString::number(Current_Impuls.n_similar_Impuls_in_rays,'f', 0) +
                            ";" + QString::number(Current_Impuls.blended,'f', 0) + "\n";

                    /*

                    {
                    std::string name_file,name_file_short,name_file_graph;
                      //имя файла данных с директорией, имя только самого файла, имя графического файла (без директории)
                    int device,n_bands; float resolution; int ray,ind;
                    double MJD;
                    QDateTime UT;
                    float Star_Time, d_T_zona, d_T_Half_H, max_Data, S_to_N, Dispers;
                    int n_similar_Impuls_in_rays,blended;} Point_Impuls_D;


                    */



                    writeStream << str1 ;
                             // Посылаем строку в поток для записи
                    fileOut_IMP_data.flush();
                  };//for (int i=0;i<count_impuls;++i)



                fileOut_IMP_data.close(); // Закрываем файл
              };//if(fileOut_IMP_data.open(QIODevice::WriteOnly | QIODevice::Text))


          }; //for(int i=0; i < counter_median_point_in_deriv; ++i)

        if (otladka==1)
          {
             writeStream << "ZAPISAL IMPULSy i, nn_imp --> name_file="
                         << i << ", " << Impulses_vec.size() << " ---> " << str_name_file
                         << "\n"; //Запишет в файл параметры

          };//if (otladka==1)

    }; //if (count_impuls>0)

} //void grav::Write_ArrayImpulses_in_file

void grav::load_head_of_file_with_data(QString name_file_of_bsadata,
                                       qbsa &bsaFile_read)
// грузим  шапку файла данных в указанную структуру данных
{
    if (name_file_of_bsadata.isEmpty()) //Не введено имя файла в строку формы
        return;

    bsaFile_read.clear();

    bsaFile_read.name_file_of_datas = name_file_of_bsadata;

    // открывает файл
    std::ifstream qinClientFile(bsaFile_read.name_file_of_datas.toStdString().c_str(), std::ios::binary);

    // если невозможно открыть файл, выйти из программы
    if (!qinClientFile)
    {
       QMessageBox::information(this, "File could not be opened ", bsaFile_read.name_file_of_datas);
       return;
    }

    unsigned int header_counts = 0;
    QStringList list, list_tmp;
    QString qnext_string;
    QString OutLine_1, date_MSK, time_MSK, date_and_time;   //(Самодуров В.А.)
    std::string next_string;
    std::string tmp_string; //(Самодуров В.А.)
    QDateTime dt; //(Самодуров В.А.)
    QErrorMessage errorMessage; //(Самодуров В.А.)

    while(getline(qinClientFile, next_string, '\n'))
    {
        qnext_string = QString::fromStdString(next_string);
        switch(++header_counts)
        {
        case 1:
            bsaFile_read.header.numpar = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).toInt();
            break;

        case 2:
            bsaFile_read.header.source = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed();
            break;

        case 3:
            bsaFile_read.header.alpha = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed();
            break;

        case 4:
            bsaFile_read.header.delta = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed();
            break;

        case 5:
            bsaFile_read.header.fcentral = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).toDouble();
            break;

        case 6:
            bsaFile_read.header.wb_total = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).toDouble();
            break;

        case 7:
            //bsaFile_read.header.date_begin = qnext_string.right(qnext_string.length() - qnext_string.indexOf("UTC") - strlen("UTC")).trimmed();
            // Нет, НЕ НАДО брать второе значение - оно есть не во всех файлах. Надо брать МСК, и потом -->UTC
            date_MSK = qnext_string.simplified();
            list_tmp= date_MSK.split(" ", QString::SkipEmptyParts);
            date_MSK=list_tmp.at(1);
            // взяли дату по МСК, потом, в case 8: перейдем -->UTC
            break;

        case 8:
            //bsaFile_read.header.time_begin = qnext_string.right(qnext_string.length() - qnext_string.indexOf("UTC") - strlen("UTC")).trimmed();
            // Нет, НЕ НАДО брать второе значение - оно есть не во всех файлах. Надо брать МСК, и потом -->UTC
            time_MSK = qnext_string.simplified();
            list_tmp= time_MSK.split(" ", QString::SkipEmptyParts);
            time_MSK=list_tmp.at(1);
            // взяли время по МСК, теперь перейдем -->UTC, вычтя 4 часа
            date_and_time=date_MSK + " " + time_MSK;
            //dt = QDateTime::fromString(date_and_time, "dd.MM.yyyy HH:mm:ss");
            dt = QDateTime::fromString(date_and_time, "dd.MM.yyyy h:mm:ss");
            //h	 -  the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
            dt = dt.addSecs(-14400);
            //Отняли из даты и времени 4 часа "медведевского" московского времени для перехода к UTC
            date_and_time = dt.toString("dd.MM.yyyy hh:mm:ss");

            //Отладочные проверочные сообщения:
            //OutLine_1=date_and_time;
            //ui->LineEdit->setText(OutLine_1);
            //errorMessage.showMessage("Жду тестового нажатия кнопки!");
            //errorMessage.exec();

            list_tmp= date_and_time.split(" ", QString::SkipEmptyParts);

            //Отладочные проверочные сообщения:
            //OutLine_1="Data=" + list_tmp.at(0) + " Time=" + list_tmp.at(1);
            //ui->LineEdit->setText(OutLine_1);
            //errorMessage.showMessage("Жду тестового 2-го нажатия кнопки!");
            //errorMessage.exec();

            bsaFile_read.header.date_begin = list_tmp.at(0);
            bsaFile_read.header.time_begin = list_tmp.at(1);
            break;

        case 9:
            bsaFile_read.header.date_end = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed();
            break;

        case 10:
            bsaFile_read.header.time_end = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed();
            break;

        case 11:
            list = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed().split(" ");
            foreach (const QString &str, list)
                bsaFile_read.header.modulus.push_back(str.toDouble());
            break;

        case 12:
            bsaFile_read.header.tresolution = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).toDouble();
            break;

        case 13:
            bsaFile_read.header.npoints = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).toInt();
            //Число временнЫх точек данных в файле (каждая такая точка - двумерный массив, содержит обычно
            // данные 48 лучей в 7 или 33 полосах , единичное значение - float формата)
            //Внимание! Число точек в файле вовсе НЕ равно в общем случае 3600 секунд, деленные на tresolution
            // - на самом деле в концовке часа - не хватает точек на отрезке от долей секунды до ~20 секунд
            break;

        case 14:
            bsaFile_read.header.nbands = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).toInt();
            //Число полос - обычно 6 или 32
            break;

        case 15:
            list = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed().split(" ");
            foreach (const QString &str, list)
                bsaFile_read.header.wbands.push_back(str.toDouble());
            break;

        case 16:
            list = qnext_string.right(qnext_string.length() - qnext_string.indexOf(" ")).trimmed().split(" ");
            foreach (const QString &str, list)
                bsaFile_read.header.fbands.push_back(str.toDouble());
            break;

        default:
            break;
        }

        if (header_counts >= bsaFile_read.header.numpar)
            break;
    }

    bsaFile_read.sizeHeader = qinClientFile.tellg();

    qinClientFile.close();

    int nCountOfModul = (bsaFile_read.header.nbands+1) * bsaFile_read.nCanal; // количество лучей в модуле, =8
    long nCountPoint = nCountOfModul * bsaFile_read.header.modulus.size();
       // -  количество значений в одной временнОй точке!
       // - оно равно произведению числа лучей на (число частот +1)
       //Оно необходимо и для считывания числа калибровочных значений
    bsaFile_read.header.nCountPoint=nCountPoint;
    double double_nCountPoint = nCountPoint;

    unsigned int countsCanal = bsaFile_read.header.modulus.size() * bsaFile_read.nCanal; // количество каналов
    npoints = bsaFile_read.header.npoints;
    //Число временнЫх точек данных в файле

    // заполнение выпадающего списка
    ui->comboBox->clear();
    ui->comboBox->addItem(" (-)");
    for (unsigned int i = 0; i <= bsaFile_read.header.nbands; i++)
        ui->comboBox->addItem(QString::number(i+1) + (i == bsaFile_read.header.nbands ? " (усред.)" : (" (" + QString::number(bsaFile_read.header.fbands[i], 'f', 2) + " МГц)")));

    ui->comboBox_2->clear();
    ui->comboBox_2->addItem(" (-)");
    for (unsigned int i = 0; i < countsCanal; i++)
        //ui->comboBox_2->addItem(QString::number(countsCanal-i) + " ("  /* Было у Торопова - в обратном порядлке нумерация*/
        ui->comboBox_2->addItem(QString::number(i+1) + " ("
                + QString::number(bsaFile_read.dec[
                      ((bsaFile_read.name_file_of_datas.indexOf("_n2_", 0, Qt::CaseInsensitive) != -1)
                      ? 6 : 0) + i/bsaFile_read.nCanal]
                                  [i%bsaFile_read.nCanal])
                                                      + "°)");
    //(Условие) ? (выражение1):(выражение2)
    // Если условие истинно, то выполняется выражение1, а если ложно, то выражение2.
    //???  != -1 ? 6 : 0   - или 6-я строка берется первой (2-я стойка), или 0-я (1-я стойка)


}
// Загрузили  шапку файла данных в указанную структуру данных


// Загрузим файл эквивалентов в соответствующие калибровочные вектора данных
void grav::load_of_file_equivalents(QString name_file_of_equivalents,unsigned int nCountPoint)
{
    if (name_file_of_equivalents.isEmpty()) //Не введено имя файла эквивалентов
        return;

    // разборка калибровочного файла
    std::ifstream inHRFile(name_file_of_equivalents.toStdString().c_str(), std::ios::binary);

    // если невозможно открыть файл, выйти из программы
    if (!inHRFile)
    {
       QMessageBox::information(this, "File could not be opened ", name_file_of_equivalents);
       return;
    }


    std::string next_string;
    //QMessageBox::information(this, "A Message from the Application", "IT IS TEST 11111111111111111111111111111111111");
    //return; // выдает во всплывающее окно тестовое сообщение на всю длину

    //qDebug() << QString::fromStdString(bsaFile_read.header.date_begin) << " " << QString::fromStdString(bsaFile_read.header.time_begin);


    //std::vector<double> v;
    //объявляем double вектор v (Самодуров В.А.)

    //for (unsigned int i = 0; i <= nCountPoint; i++)
    //    v.insert(0.0);
    //возникает disassmbler строчкой ниже, на втором проходе!
    std::vector<double> v(nCountPoint, 0.0); //Торопов М.О.! - инициализ. 0-ми (Самодуров В.А.)
    // std::vector<double> v(double_nCountPoint, 0.0);


    kalib278.clear();
    kalib2400.clear();

    long Count_string_HRFile=0;

    QRegExp rx("[,\\|{} ]");
    while(getline(inHRFile, next_string, '\n'))
    {
        Count_string_HRFile=Count_string_HRFile+1;
        if(next_string.find('{') == std::string::npos)
            continue; // шапка

        QStringList list = QString::fromStdString(next_string).split(rx, QString::SkipEmptyParts);


       //OutLine_1="NN=" + QString::number(Count_string_HRFile) + " l_0=" + list.at(0)
       //        + " 1=" + list.at(1) + " 2=" + list.at(2) +
       //        " 3=" + list.at(3) + " 4=" + list.at(4) + " 5=" + list.at(5)
       //        + " 6=" + list.at(6);
       // ui->LineEdit->setText(OutLine_1);


        //QErrorMessage errorMessage;
        //errorMessage.showMessage("Жду тестового 2-го нажатия кнопки!");
        //errorMessage.exec();



        //ui->LineEdit->setText(nCountPoint);
        for(int i=6; i < list.size(); ++i)
            v[i-6] = list.at(i).toDouble();

        //if(list.at(4).toInt() == 278)
        //    kalib278.insert(std::pair<double, std::vector<double>>(list.at(2).toDouble(), v));
        if(list.at(3).toInt() == 0)
            kalib278.insert(std::pair<double, std::vector<double>>(list.at(2).toDouble(), v));
        //Нет! Правильнее взять признак эквивалента ==0 в _третьей_ ячейке, сразу за MJD
        else if(list.at(3).toInt() == 1)
            kalib2400.insert(std::pair<double, std::vector<double>>(list.at(2).toDouble(), v));

    }
    inHRFile.close(); //Закрываем только что прочтенный нами в нужные структуры файл
}
// Загрузили файл эквивалентов в соответствующие калибровочные вектора данных

//Считываем данные в заданных рамках (времен, координат и т.д.) в небольшой кусок данных-
//на основе последовательного списка файлов данных
void grav::read_data_from_list_of_files(unsigned int &alarm_cod_of_read,
                             unsigned int &current_number_of_file_in_list,
                             QDateTime &current_time_UT,
                             double &current_time_RA,
                             double &star_time_start_end_H,
                             double &start_time_in_sec_end,
                             long &start_point,
                             unsigned int deriv,
                             double deriv_star,
                                // количество сдвиговых секунд,вычисленных из ЗВЕЗДНЫХ секунд в порции данных
                                //- реально их должно быть меньше, чтобы не было разрывов в данных
                             double add_to_deriv, //добавочные секунды для избежания различных граничных эффектов
                             float step_in_star_second,
                                //число Звездных секунд в единичном шаге данных внутри считываемого отрезка
                             float &limit_Alpha_min,
                             float &limit_Alpha_max,
                             float &limit_Delta_min,
                             float &limit_Delta_max,
                             std::string &limit_UT_min,
                             std::string &limit_UT_max,
                             int &read_from_file_or_list_names_of_files,
                             std::string &list_names_of_files)
{

    QString OutLine_1;
    QErrorMessage errorMessage; //(Самодуров В.А.)
    long n_single_points;
    double step_for_median_data_in_sec=step_in_star_second/coeff_from_Tutc_to_Tstar;
    //Интервал медианного осреднения - сразу от звездных секунд (в которых задаем изначально величину в 20 секунд)
    //привязали к обычным, временнЫм

    //double add_to_deriv=31.0;
    //Добавка 31 секунды к первоначально заявленной порции считываемых секунд данных
    //- поскольку это соответствует DM=2000 (в полной полосе частот 2.5 MHz),
    //хочется именно до этого ограничения работать, считывая дополнительные данные,
    //чтобы их можно было складывать с основными (сдвигаясь от них на дополнительные) при больших дисперсиях

    unsigned int nbands_in_file,nbands_in_file_2,need_in_File_2;
    //long tmp_ind,tmp_band,tmp_Canal,tmp_max;
    double start_time_in_sec,star_time_start_first_H,tmp,star_time_in_Hour_start_file,
            UT_start_in_MSec,UT_stop_in_MSec,UT_start_file_in_MSec,RA_start_file,RA_stop_file,RA_start_file_2,RA_stop_file_2,
            tresolution,tmp_RA;
    long npoints_in_file,needed_npoints_for_reading,ideal_npoints_in_file,npoints_in_file_2;

    QDateTime UT_start,UT_stop; //(Самодуров В.А.)
    UT_start=QDateTime::fromString(limit_UT_min.c_str(), "yyyy-MM-dd h:mm:ss");
    UT_stop=QDateTime::fromString(limit_UT_max.c_str(), "yyyy-MM-dd h:mm:ss");
    UT_start_in_MSec=UT_start.toMSecsSinceEpoch();
    UT_stop_in_MSec=UT_stop.toMSecsSinceEpoch();
    //start_time_in_sec=0;
       // стартовая секунда, с которой надо отрисовывать и обрабатывать файлы порций;
    //!!!ОПРЕДЕЛИТЬ! И выдать наружу - она там нужна!

    std::ofstream log_out;                    //создаем поток для до-записи в файл

    if (otladka==1)
       {
OutLine_1="Read_dt_000, Границы== "
            + QString::number(UT_start_in_MSec,'f',3) + ";"
            + QString::number(UT_stop_in_MSec,'f',3) + "; MyRA="
            + QString::number(current_time_RA,'f',3) + "=My; RA_min="
            + QString::number(limit_Alpha_min,'f',3) + "; RA_max="
            + QString::number(limit_Alpha_max,'f',3) + ";"
            + QString::number(current_time_RA,'f',3);
    ui->LineEdit->setText(OutLine_1);
    //errorMessage.showMessage(OutLine_1);
    //errorMessage.exec();
    };


    if (otladka==1)
     {
        writeStream << "List files, testing : limit_Alpha_min,current_time_RA,limit_Alpha_max;;;limit_UT_min,current_time_UT,limit_UT_max,current_number_of_file="
                    << limit_Alpha_min << ", MY="
                    << current_time_RA << " , " << limit_Alpha_max << ", "
                    << limit_UT_min.c_str() << ","
                    << current_time_UT.toString("yyyy-MM-dd hh:mm:ss") << ","
                    << limit_UT_max.c_str() << "," << current_number_of_file_in_list << "\n"; //Запишет в файл индекс
        fileOut.flush();
     };//if (otladka==1)


   // double toDt.toMSecsSinceEpoch()


    //ИЩЕМ ПЕРВЫЙ ПОДХОДЯЩИЙ ФАЙЛ В СПИСКЕ
    //...
    //?????????????????????????????????????????????
    //float step_in_star_second=20.0;

    need_in_File_2=0;
    int number_repeating_of_last_point=0;
    long npoints_need_from_file_1=0;
    long npoints_need_from_file_2=0;
    int number_repeating_of_last_point_2files=0;

    alarm_cod_of_read=1;

    std::string next_string;
    QDateTime UT_start_file,UT_stop_file; //(Самодуров В.А.)
    QString Name_of_file_1, Name_of_file_2, date_and_time;   //(Самодуров В.А.)
 // разборка калибровочного файла
    std::ifstream inHRFile(list_names_of_files.c_str(), std::ios::binary);
    if (!inHRFile)
    {
       QMessageBox::information(this, "File for reading is absent: ", "File HR list could not be opened ",
                                list_names_of_files.c_str());
       return;
    }
    long Count_string_HRFile=0;
    QRegExp rx("[;]");
    while((getline(inHRFile, next_string, '\n')) && (alarm_cod_of_read!=0))
    {
        Count_string_HRFile=Count_string_HRFile+1;
        if (current_number_of_file_in_list<Count_string_HRFile)//То есть этот файл мы еще не читали
        {
         QStringList list = QString::fromStdString(next_string).split(rx, QString::SkipEmptyParts);
         date_and_time = list.at(0);
//2015-09-14 8:00:00;140915_09_N1_00.pnthr;D:\DATA\GW\01\140915_09_N1_00.pnthr;32;288152;12.4928;6.03008012991812;7.03276938018629;57279.1666666667
         UT_start_file = QDateTime::fromString(date_and_time, "yyyy-MM-dd h:mm:ss");
               //h	 -  the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
         UT_start_file = UT_start_file.addSecs(-14400);
          //Отняли из даты и времени 4 часа "медведевского" московского времени для перехода к UTC
          // UT_stop_file = UT_start_file.addSecs(3600);
          //Задали конец часа
         UT_start_file_in_MSec=UT_start_file.toMSecsSinceEpoch();

         if (otladka==2)
            {
OutLine_1="Read_dt_001, Время файла,N и строка== "
                 + UT_start_file.toString("yyyy-MM-dd hh:mm:ss.zzz")  + "; File_ms="
                 + QString::number(UT_start_file_in_MSec,'f',3) + "; "
                 + QString::number(Count_string_HRFile,'f',0) + ";!!!; STR="
                 + next_string.c_str();
         ui->LineEdit->setText(OutLine_1);
         //errorMessage.showMessage(OutLine_1);
         //errorMessage.exec();
         };

         if ((UT_start_file_in_MSec >= UT_start_in_MSec) && (UT_start_file_in_MSec <= UT_stop_in_MSec))
         //Проверили невыход из границ во времени
         {
             RA_start_file=list.at(6).toDouble();
             RA_stop_file=list.at(7).toDouble();

             if (otladka==1)
                {
    OutLine_1="Read_dt_001-2!!, Check RA for== "
                     + QString::number(current_time_RA,'f',3) + "; Min_RA="
                     + QString::number(limit_Alpha_min,'f',3) + "; Max_RA="
                     + QString::number(limit_Alpha_max,'f',3) + "; Min_RA_f="
                     + QString::number(RA_start_file,'f',3) + "; Max_RA_f="
                     + QString::number(RA_stop_file,'f',3) + "; FILE="
                     + Name_of_file_1;
             ui->LineEdit->setText(OutLine_1);
             //errorMessage.showMessage(OutLine_1);
             //errorMessage.exec();
             };
             if ((current_time_RA >= limit_Alpha_min) && (current_time_RA <= limit_Alpha_max) &&
                  (current_time_RA >= RA_start_file) && (current_time_RA <= RA_stop_file))
               //Проверили невыход из границ во времени RA
              {//Читаем и работаем с данным файлом, беря его за основу (если надо, допишем из соседнего последующего)
               alarm_cod_of_read=0;
               current_time_UT=UT_start_file;
               current_number_of_file_in_list=Count_string_HRFile;
               tresolution=list.at(5).toDouble();
               npoints_in_file=list.at(4).toLong();
               Name_of_file_1=list.at(2);
               nbands_in_file=list.at(3).toInt();
               start_time_in_sec=(current_time_RA-RA_start_file)*3600.0*coeff_from_Tutc_to_Tstar;
               star_time_in_Hour_start_file=RA_start_file;
               star_time_start_first_H=RA_start_file+(start_time_in_sec/3600.0 * coeff_from_Tutc_to_Tstar);
               //=star_time_in_Hour_start_file+(start_time_in_sec/3600.0 * coeff_from_Tutc_to_Tstar);
                //высчитали звездное время начала обрабатываемого участка (с заданным сдвигом во времени (обычном) от начала файла)
               /*
               while (star_time_start_first_H > 24.0)
                 star_time_start_first_H = star_time_start_first_H -24.0;
               //Предусмотрели и поправили возможный переход через звездную полночь НО! Потом опять отнимается, потом сравнивается... Убрал
               */
               tmp=star_time_start_first_H-int(star_time_start_first_H);
               tmp=int (tmp/(step_in_star_second/3600.0) +0.5);
                   //нашли ближайшее целое число кусков по 20 секунд в дробной части часов
               star_time_start_end_H=int(star_time_start_first_H)+tmp*(step_in_star_second/3600.0)
                          -(step_in_star_second/2)/3600.0;
                 //сдвинулись на начало 20 секундного (ЗВЕЗДНЫХ секунд!) отрезка данных
                 //минус еще 10 секунд (чтобы метка 20 сек. попадала в центр)
               while (star_time_start_end_H < star_time_start_first_H)
                 star_time_start_end_H = star_time_start_end_H + (step_in_star_second/3600.0);
               //Хотим априори первую точку старта (округленную по 20 секундам) расположить после от первоначально определенной
               start_time_in_sec_end=(star_time_start_end_H-star_time_in_Hour_start_file)*3600.0/coeff_from_Tutc_to_Tstar;
               start_point =int( start_time_in_sec_end / (0.001 * tresolution) +0.5);
                       //Стартовая точка чтения данных файла, столько надо пропустить!
                       // устанавливаем позицию на НУЖНОЕ начало двоичных данных
                       //- в соответствии со сдвигом начала чтения файла на start_point временнЫх точек
               needed_npoints_for_reading =
                  int((1000/tresolution) *
                      (deriv_star + add_to_deriv)+0.5);
               if (nbands_in_file==32)
                {
                 ideal_npoints_in_file=288166;
                }
               else
                {
                 ideal_npoints_in_file=36021;
                };




               /*
               //https://stackoverflow.com/questions/17337602/how-to-get-error-message-when-ifstream-open-fails
               std::ifstream f;
               if (otladka==2)
                  {
   OutLine_1="Перед тестом чтения файла == " + Name_of_file_1;
               ui->LineEdit->setText(OutLine_1);
               errorMessage.showMessage(OutLine_1);
               errorMessage.exec();
               };
               f.open(Name_of_file_1.toStdString());
               if ( f.fail() )//проверка файла - если плохой, не берем его!
               {
                   // I need error message here, like "File not found" etc. -
                   // the reason of the failure
                   alarm_cod_of_read=1;
                   if (otladka==1)
                      {
       OutLine_1="НЕ БЕРЕМ файл == " + Name_of_file_1;
                   ui->LineEdit->setText(OutLine_1);
                   errorMessage.showMessage(OutLine_1);
                   errorMessage.exec();
                   };
               }
               else
               {
                   if (otladka==2)
                      {
       OutLine_1="ВОЗЬМЕМ файл (после проверки!)== " + Name_of_file_1;
                   ui->LineEdit->setText(OutLine_1);
                   errorMessage.showMessage(OutLine_1);
                   errorMessage.exec();
                   };

                   load_head_of_file_with_data(Name_of_file_1,bsaFile);
                   n_single_points = needed_npoints_for_reading * bsaFile.header.nCountPoint;
                    //всего единичных данных в файле-массиве
                   bsaFile.data_file.clear();
                   bsaFile.data_file.resize(n_single_points);
                   //Или bsaFile.data_file.reserve(n_single_points); ?
                    //Изменили (выставили до итогового числа точек внутри порции-кадра) размер массива

                   //Прочтем ОПЫТНУЮ порцию данных из конца файла файла
                   f.seekg(bsaFile.sizeHeader + (n_single_points-100) * sizeof(float), std::ios::beg);
                   try {
                       f.read((char*) &bsaFile.data_file[0], 10 * sizeof(float));
                         //read извлекает блоки (символов) в нужном количестве, заполняя массив bsaFile.data_file
                   }
                   catch (std::ios_base::failure& e) {
                     //std::cerr << e.what() << '\n';
                       alarm_cod_of_read=1;
                       if (otladka==1)
                          {
           OutLine_1="НЕ БЕРЕМ файл после попытки чтения == " + Name_of_file_1;
                       ui->LineEdit->setText(OutLine_1);
                       errorMessage.showMessage(OutLine_1);
                       errorMessage.exec();
                       };
                   };

                   f.close();  // Закрыли файл с данными

                   if (otladka==2)
                      {
       OutLine_1="Закрыли файл == " + Name_of_file_1;
                   ui->LineEdit->setText(OutLine_1);
                   errorMessage.showMessage(OutLine_1);
                   errorMessage.exec();
                   };

               }
               //Не срабатывает проверка...
                  */


               /*

               std::ifstream f;
               if (otladka==1)
                  {
   OutLine_1="Перед тестом чтения файла == " + Name_of_file_1;
               ui->LineEdit->setText(OutLine_1);
               errorMessage.showMessage(OutLine_1);
               errorMessage.exec();
               };
               try {
                   if (otladka==1)
                      {
       OutLine_1="Вошли в try == " + Name_of_file_1;
                   ui->LineEdit->setText(OutLine_1);
                   errorMessage.showMessage(OutLine_1);
                   errorMessage.exec();
                   };
                   f.open(Name_of_file_1.toStdString());
                   if (otladka==1)
                      {
       OutLine_1="Открыли == " + Name_of_file_1;
                   ui->LineEdit->setText(OutLine_1);
                   errorMessage.showMessage(OutLine_1);
                   errorMessage.exec();
                   };
                   load_head_of_file_with_data(Name_of_file_1,bsaFile);
                   if (otladka==1)
                      {
       OutLine_1="Загрузили заголовок == " + Name_of_file_1;
                   ui->LineEdit->setText(OutLine_1);
                   errorMessage.showMessage(OutLine_1);
                   errorMessage.exec();
                   };
                   n_single_points = needed_npoints_for_reading * bsaFile.header.nCountPoint;
                    //всего единичных данных в файле-массиве
                   bsaFile.data_file.clear();
                   bsaFile.data_file.resize(n_single_points);
                   if (otladka==1)
                      {
       OutLine_1="Назначили параметры == " + Name_of_file_1;
                   ui->LineEdit->setText(OutLine_1);
                   errorMessage.showMessage(OutLine_1);
                   errorMessage.exec();
                   };
                   f.seekg(bsaFile.sizeHeader + (n_single_points-100) * sizeof(float), std::ios::beg);
                   if (otladka==1)
                      {
       OutLine_1="Сдвинулись по стеку файла == " + Name_of_file_1;
                   ui->LineEdit->setText(OutLine_1);
                   errorMessage.showMessage(OutLine_1);
                   errorMessage.exec();
                   };
                   f.read((char*) &bsaFile.data_file[0], 10 * sizeof(float));
                     //read извлекает блоки (символов) в нужном количестве, заполняя массив bsaFile.data_file
                   if (otladka==1)
                      {
       OutLine_1="Прочли часть данных == " + Name_of_file_1;
                   ui->LineEdit->setText(OutLine_1);
                   errorMessage.showMessage(OutLine_1);
                   errorMessage.exec();
                   };
                   //throw 1;
                     // ни один оператор, следующий далее (до закрывающей скобки)
                     // выполнен не будет
                  }
               catch (...) {
                   alarm_cod_of_read=1;
                   if (otladka==1)
                      {
       OutLine_1="НЕ БЕРЕМ файл после попытки чтения == " + Name_of_file_1;
                   ui->LineEdit->setText(OutLine_1);
                   errorMessage.showMessage(OutLine_1);
                   errorMessage.exec();
                   };
               };

               if (otladka==1)
                      {
       OutLine_1="Перед закрытием файла == " + Name_of_file_1;
                   ui->LineEdit->setText(OutLine_1);
                   errorMessage.showMessage(OutLine_1);
                   errorMessage.exec();
                   };


               f.close();  // Закрыли файл с данными
               if (otladka==1)
                      {
       OutLine_1="Закрыли файл == " + Name_of_file_1;
                   ui->LineEdit->setText(OutLine_1);
                   errorMessage.showMessage(OutLine_1);
                   errorMessage.exec();
                   };

*/









               if (otladka==2)
                  {
   OutLine_1="Read_dt_002, !!!! В границах == "
                       + current_time_UT.toString("yyyy-MM-dd hh:mm:ss.zzz")  + ";"
                       + QString::number(current_time_RA,'f', 4) + "; FILE="
                       + Name_of_file_1;
               ui->LineEdit->setText(OutLine_1);
               //errorMessage.showMessage(OutLine_1);
               //errorMessage.exec();
               };




               if (((start_point+needed_npoints_for_reading)>npoints_in_file) && (alarm_cod_of_read==0))
                  //Контроль невыхода за пределы файла, если вышли - читаем следующий
                {
                   //bsaFile.header.npoints=npoints-start_point;
                   npoints_need_from_file_1=npoints_in_file-start_point;

                   if ((start_point+needed_npoints_for_reading)>ideal_npoints_in_file)
                    {//то есть все же точно заходит во второй файл
                     number_repeating_of_last_point=ideal_npoints_in_file-npoints_in_file;
                     if (getline(inHRFile, next_string, '\n'))
                     {//Прочли следующий и проверяем, подходит ли он нам
                     //Count_string_HRFile=Count_string_HRFile+1;
                      list = QString::fromStdString(next_string).split(rx, QString::SkipEmptyParts);
                      nbands_in_file_2=list.at(3).toInt();
                      RA_start_file_2=list.at(6).toDouble();
                      RA_stop_file_2=list.at(7).toDouble();
                      tmp_RA=current_time_RA+((needed_npoints_for_reading*tresolution)/3600000.0)*coeff_from_Tutc_to_Tstar;
                      if ((tmp_RA >= RA_start_file_2) && (tmp_RA <= RA_stop_file_2) && (nbands_in_file_2==nbands_in_file))
                       //Проверили невыход из границ во времени RA и совпадение числа каналов в данных
                       {
                        //Читаем и работаем с данным файлом, беря его за дополнение к предыдущему, и беря его параметры
                        need_in_File_2=1;
                        Name_of_file_2=list.at(2);
                        need_in_File_2=1;
                        npoints_in_file_2=list.at(4).toLong();
                        npoints_need_from_file_2=(start_point+needed_npoints_for_reading)-ideal_npoints_in_file;
                        if (npoints_need_from_file_2>npoints_in_file_2)
                         {
                          number_repeating_of_last_point_2files=npoints_need_from_file_2-npoints_in_file_2;
                          npoints_need_from_file_2=npoints_in_file_2;
                         }
                        else
                         {
                          number_repeating_of_last_point_2files=0;
                         };//if (npoints_need_from_file_2>npoints_in_file_2)
                       }
                      else
                       {//Файл оказался не соседним, ничего с него считывать не будем! И закончим на этом поиск
                        number_repeating_of_last_point=(start_point+needed_npoints_for_reading)-npoints_in_file;
                        //заполним остаток данных крайним значением
                        need_in_File_2=0;
                       };//if ((tmp_RA >= RA_start_file_2) && (tmp_RA <= RA_stop_file_2) && (nbands_in_file_2==nbands_in_file))
                     };//if (getline(inHRFile, next_string, '\n'))
                    }//if ((start_point+needed_npoints_for_reading)>ideal_npoints_in_file)
                   else
                    {//До соседнего файла не дотянулись, просто забиваем хвост данных крайним значением
                     number_repeating_of_last_point=(start_point+needed_npoints_for_reading)-npoints_in_file;
                     need_in_File_2=0;
                    }//else if ((start_point+needed_npoints_for_reading)>ideal_npoints_in_file)
                }//if ((start_point+needed_npoints_for_reading)>npoints_in_file)
               else
                {
                   npoints_need_from_file_1=needed_npoints_for_reading;
                }
                 //else if ((start_point+needed_npoints_for_reading)>npoints_in_file)
              }
               //if ((current_time_RA >= limit_Alpha_min) && (current_time_RA <= limit_Alpha_max) &&
               //(current_time_RA >= RA_start_file) && (current_time_RA <= RA_stop_file))
             else
              {
               //За границами нужного диапазона RA - ничего не делаем
              };//else if ((current_time_RA >= limit_Alpha_min) && (current_time_RA <= limit_Alpha_max) &&
             //(current_time_RA >= RA_start_file) && (current_time_RA <= RA_stop_file))
         }
         //if ((UT_start_file_in_MSec >= UT_start_in_MSec) && (UT_start_file_in_MSec <= UT_stop_in_MSec)
         else
         {//вышли за границы даты и времени , ничего не делаем
         };
         //else if ((UT_start_file_in_MSec >= UT_start_in_MSec) && (UT_start_file_in_MSec <= UT_stop_in_MSec)
        };
        //if (current_number_of_file_in_list<Count_string_HRFile)

    }//while(getline(inHRFile, next_string, '\n'))
    inHRFile.close(); //Закрываем только что прочтенный нами в нужные структуры файл




    //starTime - это класс, вычисляющий звездное время для предложенных параметров (см. qbsa.h) - здесь для начала файла



    //double start_time_in_sec = ui->doubleSpinBox->value();

    //double star_time_in_Hour_start_file=curStarTime.hour + curStarTime.minute/60.0 + curStarTime.second/3600.0;
    //double star_time_in_Hour_start_file=curStarTime.hour + curStarTime.minute/60 ... - это неверно!
    //Добавка считается тогда как ЦЕЛОЕ число!!!

    //ВНИМАНИЕ!!!
    //star_time_start_end_H = star_time_start_end_H + (20/3600);
    //- НЕВЕРНО, поскольку (20/3600)==0 !! Надо:
    //star_time_start_end_H = star_time_start_end_H + (20.0/3600.0);



    //int counter_median_point_in_deriv=0;
    //long ind_in_step_for_median_data=int( step_for_median_data_in_sec / (0.001 * bsaFile.header.tresolution) +0.5) ;
    //????????????????????????????????????????????



    //ЧИТАЕМ ЗАГОЛОВОК ФАЙЛА И ОПРЕДЕЛЯЕМ ПАРАМЕТРЫ
    //...

    if (alarm_cod_of_read==0)
      {
       std::vector<float> vector_add_data(bsaFile.header.nCountPoint);

       load_head_of_file_with_data(Name_of_file_1,bsaFile);
       n_single_points = needed_npoints_for_reading * bsaFile.header.nCountPoint;
        //всего единичных данных в файле-массиве
       bsaFile.header.npoints = needed_npoints_for_reading;
          //число точек (временнЫх, т.е. векторов!) в нашей порции данных будет в итоге
       bsaFile.data_file.clear();
       bsaFile.data_file.resize(n_single_points);
       //Или bsaFile.data_file.reserve(n_single_points); ?
        //Изменили (выставили до итогового числа точек внутри порции-кадра) размер массива


       if (otladka==1)
          {
OutLine_1="Перед чтением файла данных == " + Name_of_file_1;
       ui->LineEdit->setText(OutLine_1);
       //errorMessage.showMessage(OutLine_1);
       //errorMessage.exec();
       };
       //Прочтем нужную порцию данных из основного (обычно - единственного) файла
        // открываем нужный файл с данными:
       std::ifstream inClientFile(bsaFile.name_file_of_datas.toStdString().c_str(), std::ios::binary);
       inClientFile.seekg(bsaFile.sizeHeader + (start_point*bsaFile.header.nCountPoint) * sizeof(float), std::ios::beg);
         //Сдвинулись в файле на начало чтения данных (на нужный адрес, пропустив размер заголовка и размер ненужных точек)
       //!!! NO!!! inClientFile.read((char*) &bsaFile.data_file[0], npoints_need_from_file_1 * sizeof(float));
       inClientFile.read((char*) &bsaFile.data_file[0], n_single_points * sizeof(float));
         //read извлекает блоки (символов) в нужном количестве, заполняя массив bsaFile.data_file
         //(public функция-член std::basic_istream)
         //СчитаЛИ данные из основного файла
       inClientFile.close();
       // Закрыли файл с данными

       if (otladka==1)
          {
OutLine_1="Благополучно прочли (и уже закрыли) файл данных == " + Name_of_file_1;
       ui->LineEdit->setText(OutLine_1);
       //errorMessage.showMessage(OutLine_1);
       //errorMessage.exec();
       };

       if (otladka==1)
        {
           writeStream << "PORCIA  in lenta: sizeHeader, start_time_in_sec_end, start_point, size_point, n_points="
                       << bsaFile.sizeHeader << ", "
                       << start_time_in_sec_end << " : start_point=" << start_point << ", size_point="
                       << bsaFile.header.nCountPoint << ","
                       << npoints_need_from_file_1 << "\n"; //Запишет в файл индекс
           fileOut.flush();
        };//if (otladka==1)



       if (number_repeating_of_last_point>0)
        {//Необходимо number_repeating_of_last_point раз добавить-скопировать крайнюю временнУю точку к данным
           vector_add_data.clear();
           //std::copy(bsaFile.data_file.end()-bsaFile.header.nCountPoint, bsaFile.data_file.end()-1,std::back_inserter(vector_add_data));
           //Как в http://forum.vingrad.ru/topic-332829.html
           std::copy(bsaFile.data_file.end()-bsaFile.header.nCountPoint, bsaFile.data_file.end(), vector_add_data.begin());
           //http://ru.cppreference.com/w/cpp/algorithm/copy
           for (unsigned int i=1; i <= number_repeating_of_last_point; ++i)
             bsaFile.data_file.insert( bsaFile.data_file.end(), vector_add_data.begin(), vector_add_data.end() );
        };//if (number_repeating_of_last_point>0)

       qbsa bsaFile_2;
       if (need_in_File_2!=0)
         {
           load_head_of_file_with_data(Name_of_file_2,bsaFile_2);
           long n_single_points_2 = npoints_need_from_file_2 * bsaFile_2.header.nCountPoint;
            //всего единичных данных в файле-массиве
           bsaFile_2.header.npoints = npoints_need_from_file_2;
              //число точек (временнЫх, т.е. векторов!) в нашей порции данных будет в итоге
           bsaFile_2.data_file.clear();
           bsaFile_2.data_file.resize(n_single_points_2);
           //Или bsaFile.data_file.reserve(n_single_points); ?
            //Изменили (выставили до итогового числа точек внутри порции-кадра) размер массива

           //Прочтем нужную порцию данных из основного (обычно - единственного) файла
            // открываем нужный файл с данными:
           std::ifstream inClientFile(bsaFile_2.name_file_of_datas.toStdString().c_str(), std::ios::binary);
           inClientFile.seekg(bsaFile_2.sizeHeader, std::ios::beg);
             //Сдвинулись в файле на начало чтения данных (пропустив размер заголовка и начав с 1-й же точки)
           //NO!!! inClientFile.read((char*) &bsaFile_2.data_file[0], npoints_need_from_file_2 * sizeof(float));
           inClientFile.read((char*) &bsaFile.data_file[0], n_single_points_2 * sizeof(float));
             //read извлекает блоки (символов) в нужном количестве, заполняя массив bsaFile_2.data_file
             //(public функция-член std::basic_istream)
             //СчитаЛИ данные из основного файла
           inClientFile.close();
           // Закрыли файл с данными

           bsaFile.data_file.insert( bsaFile.data_file.end(), bsaFile_2.data_file.begin(), bsaFile_2.data_file.end() );
           //Дополнили данные основного файла - данными из следующего файла

           if (number_repeating_of_last_point_2files>0)
            {//Необходимо number_repeating_of_last_point раз добавить-скопировать крайнюю временнУю точку к данным
               vector_add_data.clear();
               //std::copy(bsaFile.data_file.end()-bsaFile.header.nCountPoint, bsaFile.data_file.end()-1,std::back_inserter(vector_add_data));
               //Как в http://forum.vingrad.ru/topic-332829.html
               std::copy(bsaFile_2.data_file.end()-bsaFile_2.header.nCountPoint, bsaFile_2.data_file.end(), vector_add_data.begin());
               //http://ru.cppreference.com/w/cpp/algorithm/copy
               for (unsigned int i=1; i <= number_repeating_of_last_point_2files; ++i)
                 bsaFile.data_file.insert( bsaFile.data_file.end(), vector_add_data.begin(), vector_add_data.end() );
            };//if (number_repeating_of_last_point>0)


         };//if (need_in_File_2!=0)



       //СчитаЛИ данные из файла
       //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

       if ((otladka==1) )
        {
         writeStream << "\n READ from LIST alarm_cod_of_read=0 CHECK BASE DATE bsaFile.data_file[0],  bsaFile.data_file[bsaFile.data_file.size()-1], size : "
                   << bsaFile.data_file[0] << ", " << bsaFile.data_file[bsaFile.data_file.size()-1] << "," << bsaFile.data_file.size()
                   << ",\n PARAM npoints_need_from_file_1, needed_npoints_for_reading, number_repeating_of_last_point,need_in_File_2,npoints_need_from_file_2,number_repeating_of_last_point_2files=="
                   << npoints_need_from_file_1 << ", "
                   << needed_npoints_for_reading << ", "
                   << number_repeating_of_last_point << ", "
                   << need_in_File_2 << ", "
                   << npoints_need_from_file_2 << ", "
                    << number_repeating_of_last_point_2files
                   << "\n\n"; //Запишет в файл параметры
        };

      };//if (alarm_cod_of_read==0)

    if (otladka==2)
       {
OutLine_1="Read_dt_END, код и что из файла ==!!-"
            + QString::number(alarm_cod_of_read,'f', 0) + "-!!;"
            + current_time_UT.toString("yyyy-MM-dd hh:mm:ss.zzz")  + ";"
            + QString::number(bsaFile.header.nCountPoint,'f', 0) + ";"
            + QString::number(bsaFile.header.npoints,'f', 0) + "; FILE="
            + Name_of_file_1;
    ui->LineEdit->setText(OutLine_1);
    errorMessage.showMessage(OutLine_1);
    errorMessage.exec();
    };

}//void grav::read_data_from_list_of_files






//Делаем обработку большого куска данных - считывая данные по маленьким кускам - калибровка, нахождение медианных, поиск дисперсий
void grav::base_processing_of_file(unsigned int max_i,
                             //bool stopThread,
                             unsigned int deriv,
                             float step_in_star_second,
                             long &start_point,
                             double &start_time_in_sec_end,
                             double &star_time_in_Hour_start_file,
                             double &star_time_start_end_H,
                             int param_1_otladka,
                              //раз в сколько точек выводим в лог)
                             int &do_dispers,
                             float &limit_dispersion_min,
                             float &limit_dispersion_max,
                             float &limit_myltiply,
                             float &limit_sigma,
                             float &multiply_y,
                             float &scale_T,
                             float &add_multiply,
                             std::string &out_data_dir,
                             std::string &dispers_data_file,
                             float &limit_Alpha_min,
                             float &limit_Alpha_max,
                             float &limit_Delta_min,
                             float &limit_Delta_max,
                             std::string &limit_UT_min,
                             std::string &limit_UT_max,
                             int &read_from_file_or_list_names_of_files,
                             std::string &list_names_of_files,
                             std::string &names_of_file_EQ)
{
//+++++++++++++++Начало обработки - прохода по заданным временным отрезкам+++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    long n_single_points, tmp_long;
    std::string name_file,name_file_short;
    int device,n_bands,ray,ind;
    float resolution;
    double MJD;
    QDateTime UT;
    QString OutLine_1, date_and_time;
    float Star_Time, d_T_zona, d_T_Half_H, max_Data, S_to_N, Dispers;
    int n_similar_Impuls_in_rays;
    QErrorMessage errorMessage; //(Самодуров В.А.)
    std::string tmp_string; //(Самодуров В.А.)

    // загрузка данных
    QDateTime start_time_program = QDateTime::currentDateTime();
    //Захватили время старта подпрограммы
    QDateTime stop_time_program, prev_stop_time_program;
    QDateTime current_time_UT=QDateTime::fromString(limit_UT_min.c_str(), "yyyy-MM-dd h:mm:ss");
    float counter_ms_from_start;

    long counter_otladka=0;
    float add_tmp=0;
    //float step_in_star_second=20.0;
    double step_for_median_data_in_sec=step_in_star_second/coeff_from_Tutc_to_Tstar;
    //Интервал медианного осреднения - сразу от звездных секунд (в которых задаем изначально величину в 20 секунд)
    //привязали к обычным, временнЫм
    double deriv_star = deriv  / coeff_from_Tutc_to_Tstar;
    // количество сдвиговых секунд,вычисленных из ЗВЕЗДНЫХ секунд в порции данных
    //- реально их должно быть меньше, чтобы не было разрывов в данных
    double add_to_deriv=31.0;
    //Добавка 31 секунды к первоначально заявленной порции считываемых секунд данных
    //- поскольку это соответствует DM=2000 (в полной полосе частот 2.5 MHz),
    //хочется именно до этого ограничения работать, считывая дополнительные данные,
    //чтобы их можно было складывать с основными (сдвигаясь от них на дополнительные) при больших дисперсиях
    int counter_median_point_in_deriv=0;
    unsigned int countsCanal;
    int nCountOfModul,size_point,counter_read_from_list_of_files;
    long ind_in_step_for_median_data,tmp_ind,tmp_band,tmp_Canal,tmp_max,nCountPoint;
    double x1, x2, y1, y2;
    std::ofstream log_out;                    //создаем поток для до-записи в файл

    int counter_bad_data, counter_bad_dispers_data;
    float limit_flux_min,limit_flux_max,limit_dispers,cod_check_dispers;
    limit_flux_min=300.0;
    limit_flux_max=40000.0;
    limit_dispers=200.0;


    counter_read_from_list_of_files=0;
    int read_eq=0;
    //НАЧАЛО Находим-Запоминаем в переменные общие данные для импульсов:
    //std::string name_file,name_file_short;   ++, ++
    //int device,n_bands,resolution    ++, ++, ++
    if (read_from_file_or_list_names_of_files==0)
     //Чтение из предложенного файла
     {
       name_file=bsaFile.name_file_of_datas.toStdString().c_str();
       const char separator[]=":/\\";
       //char *S = new char[bsaFile.name_file_of_datas.toStdString().Length()];
       char *S = new char[name_file.size()];
       //char S[name_file.size()]="";  - не работает
       strcpy( S, name_file.c_str() );
       //S[name_file.size()]=name_file;
       //S=name_file.c_str();
       //S = bsaFile.name_file_of_datas.toStdString().c_str();
       char *Ptr=NULL;
       Ptr=strtok(S,separator); //Исходная строка будет изменена
       while (Ptr)
        {
         name_file_short=Ptr; //выводим очередную часть строки из строки в искомую переменную
         Ptr=strtok(0,separator); //указываем на новый токен
        };
       device = (bsaFile.name_file_of_datas.indexOf("_n2_", 0, Qt::CaseInsensitive) != -1) ? 2 : 1;
       //(Условие) ? (выражение1):(выражение2)
       // Если условие истинно, то выполняется выражение1, а если ложно, то выражение2.
       // Берем либо 2-ю стойку (если в названии есть "_n2_"), либо по умолчанию 1-ю.
       resolution=bsaFile.header.tresolution;
       n_bands=bsaFile.header.nbands;
       //КОНЕЦ

    /*
    QProgressDialog* progress_ptr = new QProgressDialog("Формируем файлы ...", "Отмена", 0, 100, this);
    connect(progress_ptr, SIGNAL(canceled()), this, SLOT(on_progress_cancel()));
    progress_ptr->setGeometry(this->geometry().left() + 50, this->geometry().top() + 20, 300, 25);
    progress_ptr->setWindowModality(Qt::ApplicationModal);

    progress_ptr->show();
        //Показывает прогресс обработки файлов в меню на экране
*/

       ind_in_step_for_median_data=int( step_for_median_data_in_sec / (0.001 * bsaFile.header.tresolution) +0.5) ;

       bsaFile.header.npoints = npoints;
                   //число точек (временнЫх, т.е. векторов!) в файле данных, в идеале = 36000 или 288000
       nCountOfModul = (bsaFile.header.nbands+1) * bsaFile.nCanal;
                   // количество (-лучей-) точек данных в одном модуле из 8 лучей - обычно 56 или 264
       nCountPoint = nCountOfModul * bsaFile.header.modulus.size();
        // количество (-лучей-) точек данных во всех модулях - ТО ЕСТЬ - сколько единичных точек в одной временнОй точке
        //- обычно или 48(лучей)*7(полос)= 336 или 48*33= 1584
        //--long nCounts = bsaFile.header.npoints*nCountPoint;
              // количество _единичных_ точек данных во всём файле, в идеале 36000*336 или 288000*1584  НЕ ИСПОЛЬЗУЕТСЯ!
       countsCanal = bsaFile.header.modulus.size() * bsaFile.nCanal;
                           // количество (-каналов-) лучей диаграммы БСА в файле - практически всегда = 48

       size_point = (bsaFile.header.nbands+1) * bsaFile.nCanal * bsaFile.header.modulus.size();
        //Число данных файла в 1 временной точке, для 6 полос и 48 лучей =336
        //Надо будет сдвинуться на НУЖНОЕ начало двоичных данных, с которого начнем считывание, примерно так:
        // пример:  inClientFile.seekg(bsaFile.sizeHeader + (start_point*size_point) * sizeof(float), std::ios::beg);



       //!!!!!
       // устанавливаем позицию на начало двоичных данных
       //inClientFile.seekg(bsaFile.sizeHeader, std::ios::beg); - ПОТОМ!
     }//if (read_from_file_or_list_names_of_files==0)
        //Чтение из предложенного файла
    else
     {
        max_i=1; //Жестко выставляем число временных интервалов равным единице
        //- поскольку реальные изменения будут в повторящих проходах по новым
        //ЗАНОВО считываемым данным из файлов списка
     };//else if (read_from_file_or_list_names_of_files==0)

    unsigned int alarm_cod_of_read=0;
    unsigned int current_number_of_file_in_list=0;
    double current_time_RA=limit_Alpha_min;


    if (otladka==1)
       {
    OutLine_1="Pnt001 " + bsaFile.name_file_of_datas + " max_i="
    +  QString::number(max_i)  + ";"
    + current_time_UT.toString("yyyy-MM-dd hh:mm:ss.zzz")  + ";"
    + limit_UT_min.c_str()  + ";"
    + limit_UT_max.c_str()  + ","
    + QString::number(read_from_file_or_list_names_of_files,'f', 0);
    ui->LineEdit->setText(OutLine_1);
    //errorMessage.showMessage(OutLine_1);
    //errorMessage.exec();
    };

    unsigned int i=0;

    if (otladka==1)
     {
        writeStream << "\n FROM LIS OR FILE? pered i : file_or_list, number_of_file_in_list, alarm_cod_of_read,i,limit_Alpha_min,limit_Alpha_max="
                    << read_from_file_or_list_names_of_files << " , "
                    << current_number_of_file_in_list << ", "
                    << alarm_cod_of_read << " , " << i << ", " << limit_Alpha_min << ", "
                    << limit_Alpha_max << ", "
                     "\n"; //Запишет в файл индекс
        fileOut.flush();
     };//if (otladka==1)

    //!! for (unsigned int i=0; i < max_i && !stopThread; i++)
    while ( (i< max_i) && (alarm_cod_of_read==0) && (!stopThread)) //Заменили цикл на предусловие, чтобы смочь отслеживать лист файлов
      //i здесь - номер порции для обработки (обычно порции по 4 минуты, т.е. до i<15)
      {
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //ФАЗА 1: определяем вводные параметры



        if (read_from_file_or_list_names_of_files==0)
         //Чтение из предложенного файла
         {
          start_point =(int) ( (start_time_in_sec_end + i*deriv_star ) / (0.001 * bsaFile.header.tresolution) +0.5);
          //Стартовая точка чтения данных файла, столько надо пропустить!
          //start_time_in_sec_end - это поправленное значение vs то, что введено "руками"
          //(к округленному старту звездного времени)
          //И: видно, что мы смещаемся не на временнЫе заданные интервалы deriv секунд, а на звездные deriv_star!

          bsaFile.header.npoints =
           int((1000/bsaFile.header.tresolution) *
               (deriv_star + add_to_deriv)+0.5);
               //(deriv_star + (add_to_deriv/coeff_from_Tutc_to_Tstar))+0.5);
           //- количество временнЫх точек в порции, добавление add_to_deriv - для избегания граничных эффектов
           //Число точек файла в данном куске из deriv + add_to_deriv секунд - хранится теперь
           //в изменившемся bsaFile.header.npoints,
           //а вот главное, общее число точек в файле - число npoints - не трогаем!
          //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

          if (otladka==2)
           {
        OutLine_1="Pnt002 npoints=" +  QString::number(bsaFile.header.npoints);
        ui->LineEdit->setText(OutLine_1);
        //errorMessage.showMessage(OutLine_1);
        //errorMessage.exec();
           };

          //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
          //Считаем данные из файла
          if ((start_point+bsaFile.header.npoints)>npoints)
           //Контроль невыхода за пределы файла
           {bsaFile.header.npoints=npoints-start_point;};
          n_single_points = bsaFile.header.npoints * nCountPoint;
           //всего единичных данных в файле-массиве
          bsaFile.data_file.resize(n_single_points);
                //Изменили (уменьшили до числа точек внутри порции-кадра) размер массива,
                     //перемножили временные на число в одной временной
                // Для целого файла - устанавливали количество _единичных_ точек данных nCounts во всём файле,
                //в идеале 36000*336 или 288000*1584

          // ВВОДИМ и Назначаем имя файлу с данными:
          std::ifstream inClientFile(bsaFile.name_file_of_datas.toStdString().c_str(), std::ios::binary);
          inClientFile.seekg(bsaFile.sizeHeader + (start_point*size_point) * sizeof(float), std::ios::beg);
          //Сдвинулись в файле на начало чтения данных (на нужный адрес, пропустив размер заголовка
          //и размер ненужных точек)

          if (otladka==2)
           {
              writeStream << "PORCIA  i, sizeHeader, start_time_in_sec_end, start_point, size_point, n_points="
                          << i << ",  "
                          << bsaFile.sizeHeader << ", "
                          << start_time_in_sec_end << " : start_point=" << start_point << ", size_point="
                          << size_point << ","
                          << n_single_points << "\n"; //Запишет в файл индекс
              fileOut.flush();
           };//if (otladka==1)


          inClientFile.read((char*) &bsaFile.data_file[0], n_single_points * sizeof(float));
          //read извлекает блоки (символов) в нужном количестве, заполняя массив bsaFile.data_file
          //(public функция-член std::basic_istream)
          inClientFile.close();
          // Закрыли файл с данными
        //СчитаЛИ данные из файла
         }//if (read_from_file_or_list_names_of_files==0)
        //Чтение из предложенного файла
        else
         {//Чтение из списка файлов с выбором нужного в данной точке

            if (otladka==2)
               {
OutLine_1="Pnt001-a AFTER read_data_from_list_of_files: UT_curr,cur_RA,list_of_files,start_time_in_sec_end ,start_point, deriv,deriv_star,add_to_deriv,step_in_star_second,limit_Alpha_min,limit_Alpha_max,limit_UT_min,limit_UT_max,read_from_file_or== "
                                    + current_time_UT.toString("yyyy-MM-dd hh:mm:ss.zzz")  + ";"
                                    + QString::number(current_time_RA,'f', 4)  + ";"
                                    + list_names_of_files.c_str()  + ";"
                                    + QString::number(start_time_in_sec_end,'f', 2) + ";"
                                    + QString::number(start_point,'f', 0) + ";"
                                    + QString::number(deriv,'f', 2) + ";"
                                    + QString::number(deriv_star,'f', 2) + ","
                                    + QString::number(add_to_deriv,'f', 1) + ","
                                    + QString::number(step_in_star_second,'f', 1) + ","
                                    + QString::number(limit_Alpha_min,'f', 4) + ","
                                    + QString::number(limit_Alpha_max,'f', 4) + ","
                                    + limit_UT_min.c_str()  + ";"
                                    + limit_UT_max.c_str()  + ","
                                    + QString::number(read_from_file_or_list_names_of_files,'f', 0);
            ui->LineEdit->setText(OutLine_1);
            //errorMessage.showMessage(OutLine_1);
            //errorMessage.exec();
            };
             //if (otladka==2)

            /* alarm_cod_of_read=1;
            //!!while ((alarm_cod_of_read==1) && (current_time_RA<limit_Alpha_max))
              { */
                read_data_from_list_of_files(alarm_cod_of_read,current_number_of_file_in_list,
                                         current_time_UT,
                                         current_time_RA,
                                         star_time_start_end_H,
                                         start_time_in_sec_end,
                                         start_point,
                                         deriv,deriv_star,
                                            // количество сдвиговых секунд,вычисленных из ЗВЕЗДНЫХ секунд в порции данных
                                            //- реально их должно быть меньше, чтобы не было разрывов в данных
                                         add_to_deriv, //добавочные секунды для избежания различных граничных эффектов
                                         step_in_star_second,
                                            //число Звездных секунд в единичном шаге данных внутри считываемого отрезка
                                         limit_Alpha_min,limit_Alpha_max,limit_Delta_min,limit_Delta_max,
                                         limit_UT_min,limit_UT_max,
                                         read_from_file_or_list_names_of_files,
                                         list_names_of_files);
                //НАДО: start_point + , start_time_in_sec_end +, ind_in_step_for_median_data +, nCountPoint +,
                //countsCanal +, size_point + - это дубль nCountPoint, name_file_short +, device+

                if (otladka==1)
                 {
                    writeStream << "AFTER read_data_from_list_of_files result : n_flie_in_list, current_time_RA,current_time_UT,current_number_of_file,alarm_cod_of_read="
                                << current_number_of_file_in_list << ", "
                                << current_time_RA << " , " << limit_Alpha_max << ", "
                                << current_time_UT.toString("yyyy-MM-dd hh:mm:ss") << ","
                                << current_number_of_file_in_list << ", COD="<< alarm_cod_of_read <<  "\n"; //Запишет в файл индекс
                    fileOut.flush();
                 };//if (otladka==1)

                if ((otladka==1) )// && (counter_otladka>=292))
                   {
                log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
                log_out  << bsaFile.name_file_of_datas.toStdString()  << " : posle sdviga RA===: " ;
                // сама запись
                log_out << ", current_time_RA="
                        << current_time_RA << " , limit_Alpha_min=" << limit_Alpha_min
                        << " , limit_Alpha_max=" << limit_Alpha_max << "\n";

                log_out.close();      // закрываем файл
                };


                if (alarm_cod_of_read==1)
                  {
                    current_time_RA=current_time_RA+(deriv/3600.0);
                    current_number_of_file_in_list=0;
                    //Пытаемся зацепиться на новом рубеже RA, заново читая от начала список
                    read_data_from_list_of_files(alarm_cod_of_read,current_number_of_file_in_list,
                                             current_time_UT,
                                             current_time_RA,
                                             star_time_start_end_H,
                                             start_time_in_sec_end,
                                             start_point,
                                             deriv,deriv_star,
                                                // количество сдвиговых секунд,вычисленных из ЗВЕЗДНЫХ секунд в порции данных
                                                //- реально их должно быть меньше, чтобы не было разрывов в данных
                                             add_to_deriv, //добавочные секунды для избежания различных граничных эффектов
                                             step_in_star_second,
                                                //число Звездных секунд в единичном шаге данных внутри считываемого отрезка
                                             limit_Alpha_min,limit_Alpha_max,limit_Delta_min,limit_Delta_max,
                                             limit_UT_min,limit_UT_max,
                                             read_from_file_or_list_names_of_files,
                                             list_names_of_files);
                  }
                else
                  {
                    if (otladka==2)
                              {
                                 //str1 = now.toString;
                                 writeStream << "\n POSLE!!! read_data_from_list_of_files "
                                 << bsaFile.name_file_of_datas << "  filetime=" ; //Запишет в файл параметры
                                 writeStream << bsaFile.header.date_begin << " nCountPoint="
                                             << bsaFile.header.time_begin << " nCountPoint="
                                             << bsaFile.header.nCountPoint << " tresolution="
                                             << bsaFile.header.tresolution << " npoints="
                                             << bsaFile.header.npoints << " nbands="
                                             << bsaFile.header.nbands << "\n\n";
                                 //Запишет в файл системное время
                                 fileOut.flush();
                               };//if (otladka==1)

                  };


            /*  };//while ((alarm_cod_of_read==1) && (current_time_RA<limit_Alpha_max))
             */

            if (otladka==2)
               {
                 OutLine_1="Pnt001-b AFTER read_data_from_list_of_files: UT_curr,cur_RA,start_time_in_sec_end ,start_point, deriv,deriv_star,add_to_deriv,step_in_star_second !!! New !!! alarm_cod_of_read,current_number_of_file_in_list == "
                    + current_time_UT.toString("yyyy-MM-dd hh:mm:ss.zzz")  + ";"
                    + QString::number(current_time_RA,'f', 4) + ";"
                    + QString::number(start_time_in_sec_end,'f', 2) + ";"
                    + QString::number(start_point,'f', 0) + ";"
                    + QString::number(deriv,'f', 2) + ";"
                    + QString::number(deriv_star,'f', 2) + ","
                    + QString::number(add_to_deriv,'f', 1) + ","
                    + QString::number(step_in_star_second,'f', 1) + " !!! New !!! "
                    + QString::number(alarm_cod_of_read,'f', 0) + ","
                    + QString::number(current_number_of_file_in_list,'f', 0);
                 ui->LineEdit->setText(OutLine_1);
                 //errorMessage.showMessage(OutLine_1);
                 //errorMessage.exec();
                };
             //if (otladka==2)

            //Запись в конец файла:
            //std::ofstream log_out;                    //создаем поток
            log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
            log_out  << bsaFile.name_file_of_datas.toStdString()  << " N=" << current_number_of_file_in_list
                     << " ; RA=" << current_time_RA  << "\n";      // сама запись
            log_out.close();      // закрываем файл

            ind_in_step_for_median_data=int( step_for_median_data_in_sec / (0.001 * bsaFile.header.tresolution) +0.5) ;
            nCountPoint=bsaFile.header.nCountPoint;
            countsCanal = bsaFile.header.modulus.size() * bsaFile.nCanal;
                                // количество (-каналов-) лучей диаграммы БСА в файле - практически всегда = 48
            n_single_points = bsaFile.header.npoints * nCountPoint;
            size_point = bsaFile.header.nCountPoint;// (bsaFile.header.nbands+1) * bsaFile.nCanal * bsaFile.header.modulus.size();
             //Число данных файла в 1 временной точке, для 6 полос и 48 лучей =336
            name_file=bsaFile.name_file_of_datas.toStdString().c_str();


            starTime curStarTime(bsaFile.header.date_begin.mid(6,4).toInt(), bsaFile.header.date_begin.mid(3,2).toInt(),
                                     bsaFile.header.date_begin.mid(0,2).toInt(),
                                     bsaFile.header.time_begin.mid(0,bsaFile.header.time_begin.length() == 7 ? 1 : 2).toInt(),
                                     0.0, 0.0, 2.50875326667, 0);
                //starTime - это класс, вычисляющий звездное время для предложенных параметров (см. qbsa.h) - здесь для начала файла
            star_time_in_Hour_start_file=curStarTime.hour + curStarTime.minute/60.0 + curStarTime.second/3600.0;
                //double star_time_in_Hour_start_file=curStarTime.hour + curStarTime.minute/60 ... - это неверно!
                //Добавка считается тогда как ЦЕЛОЕ число!!!
            //star_time_start_first_H=star_time_in_Hour_start_file+(start_time_in_sec/3600.0 * coeff_from_Tutc_to_Tstar);
                 //высчитали звездное время начала обрабатываемого участка (с заданным сдвигом во времени (обычном) от начала файла)
            //start_time_in_sec_end=(star_time_start_end_H-star_time_in_Hour_start_file)*3600.0/coeff_from_Tutc_to_Tstar;





            if (otladka==1)
               {

                //tmp_string=bsaFile.name_file_of_datas.toStdString();
                //OutLine_1=bsaFile.name_file_of_datas  +
                OutLine_1="Pnt001-C param file: name_file,tresolution,nCountPoint,countsCanal, size_point == "
                    //+ tmp_string.c_str() +
                    + QString::number(bsaFile.header.tresolution,'f', 4) + ";"
                    + QString::number(nCountPoint,'f', 0) + ";"
                    + QString::number(countsCanal,'f', 0) + ";"
                    + QString::number(size_point,'f', 0);
            ui->LineEdit->setText(OutLine_1);
            //errorMessage.showMessage(OutLine_1);
            //errorMessage.exec();
            };

            const char separator[]=":/\\";
            //char *S = new char[bsaFile.name_file_of_datas.toStdString().Length()];
            char *S = new char[name_file.size()];
            //char S[name_file.size()]="";  - не работает
            strcpy( S, name_file.c_str() );
            //S[name_file.size()]=name_file;
            //S=name_file.c_str();
            //S = bsaFile.name_file_of_datas.toStdString().c_str();
            char *Ptr=NULL;
            Ptr=strtok(S,separator); //Исходная строка будет изменена
            while (Ptr)
             {
              name_file_short=Ptr; //выводим очередную часть строки из строки в искомую переменную
              Ptr=strtok(0,separator); //указываем на новый токен
             };
            device = (bsaFile.name_file_of_datas.indexOf("_n2_", 0, Qt::CaseInsensitive) != -1) ? 2 : 1;
            resolution=bsaFile.header.tresolution;
            n_bands=bsaFile.header.nbands;

            if (otladka==1)
               {
                 OutLine_1="Pnt001-D pered Equ: name_file_short,device,counter_read_from_list_of_files,names_of_file_EQ == "
                    //+ name_file_short  + ";"
                    + QString::number(counter_read_from_list_of_files,'f', 0) + ";"
                    + QString::number(device,'f', 0) + ","
                    + QString::number(counter_read_from_list_of_files,'f', 0) + ";"
                    + names_of_file_EQ.c_str();
                 ui->LineEdit->setText(OutLine_1);
                 //errorMessage.showMessage(OutLine_1);
                 //errorMessage.exec();
               };

            if ((read_eq==0) && (alarm_cod_of_read==0))//Не читали раньше и что-то прочли сейчас
             {
              QString name_file_of_equivalents = names_of_file_EQ.c_str();
              load_of_file_equivalents(name_file_of_equivalents,nCountPoint);
              read_eq=1;
             };

            counter_read_from_list_of_files=counter_read_from_list_of_files+1;

            /*
            current_time_RA=current_time_RA+(deriv/3600.0);
            if (current_time_RA>limit_Alpha_max)
              {
                current_time_RA=limit_Alpha_min;
                current_number_of_file_in_list=current_number_of_file_in_list+1;
              };
              */

         };//else if (read_from_file_or_list_names_of_files==0)
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        if (alarm_cod_of_read==0)
        {//Только если код тревоги равен 0 , продолжим! Он равен нулю ВСЕГДА, когда читаем из файла,
         //и - если работаем по списку и при этом только что прочли нужный файл.
          //Если же во втором случае ничего не нашли и не прочли - делаем аварийный выход
        if (otladka==1)
           {
        tmp_long=n_single_points;
        OutLine_1="Pnt003 Read from file, n_single_points=" +  QString::number(tmp_long);
        ui->LineEdit->setText(OutLine_1);
        //errorMessage.showMessage(OutLine_1);
        //errorMessage.exec();
        };

        if (otladka==1)
           {
 OutLine_1="Перед началом обработки данных для == " + bsaFile.name_file_of_datas;
        ui->LineEdit->setText(OutLine_1);
        //errorMessage.showMessage(OutLine_1);
        //errorMessage.exec();
        };

        int min_start=(int)(i*deriv_star / 60 + (start_time_in_sec_end / 60));
        double sec_start=start_time_in_sec_end + i*deriv_star - min_start*60;
        mjd_start = Mjd(bsaFile.header.date_begin.mid(6,4).toInt(),
                             bsaFile.header.date_begin.mid(3,2).toInt(),
                             bsaFile.header.date_begin.mid(0,2).toInt(),
                             bsaFile.header.time_begin.mid(0, bsaFile.header.time_begin.length() == 7 ? 1 : 2).toInt(),
                                                           min_start, sec_start);
          //посчитали Mjd старта отрисовки из UTC
          //Считаем как starTime(int Year = 2000, int Month = 1, int Date = 1, int hr = 0,
          //int min = 0, double sec = 0, double longitude_in_h = 2.50875326667, int d_h_from_UT = -4)

        starTime curStarTime(bsaFile.header.date_begin.mid(6,4).toInt(),
                             bsaFile.header.date_begin.mid(3,2).toInt(),
                             bsaFile.header.date_begin.mid(0,2).toInt(),
                             bsaFile.header.time_begin.mid(0, bsaFile.header.time_begin.length() == 7 ? 1 : 2).toInt(),
                                                           min_start, sec_start, 2.50875326667, 0);
          //посчитали звездное время старта отрисовки из UTC и долготы Пущино
        pStarTime = &curStarTime;// - и захватили его в указатель


        QString date_and_time=bsaFile.header.date_begin + " " + bsaFile.header.time_begin;
        QDateTime now = QDateTime::fromString(date_and_time, "dd.MM.yyyy HH:mm:ss");
        now = now.addMSecs((start_time_in_sec_end + i*deriv_star)*1000);
               // now = now.addMSecs((int)(start_time_in_sec*1000));
          //Прибавили число секунд от старта файла с учетом сдвига по порциям - через прибавление миллисекунд
        now.setTimeSpec(Qt::UTC);
           //Определили UTC старта отсчета

        curTime = now.toTime_t();
           //Текущее время - взяли как UTC,превратили в  curTime  - это UNIX секунды

                //При входе в runSaveFile у нас есть глобальные переменные curTime , bsaFile.header.npoints
                total = 0.0;
       if (otladka==1)
                 {
                    //str1 = now.toString;
                    writeStream << "\n DO Processing_File total, " << total << "  time=" ; //Запишет в файл параметры
                    str1 = now.toString("yyyy.MM.dd hh:mm:ss.zzz");
                    writeStream.setRealNumberNotation(QTextStream::FixedNotation);
                    writeStream << str1 << "=in sec=" << curTime << "\n\n"; //Запишет в файл системное время
                    fileOut.flush();
                  };//if (otladka==1)


        int otladka_add_1=0;
         //Для добавочного отладочного коэффициента, который будет употреблен в калибровочной функции
         //Сбросили на всякий случай значение вывода отладочных значений внутри калибровки к НЕ-выводу
        double calib_data_T; //Переменная для хранения текущих откалиброванных данных (в Кельвинах)
        double tresolution = 0.001 * bsaFile.header.tresolution;
         //реальное временнОе разрешение по 1 точке - в секундах
        unsigned int startBand = 0;
        unsigned int finishBand = bsaFile.header.nbands;
        int startBand_process = -1;
        unsigned int finishBand_process = 0;

        float epsilon;
        //Используем для сохранения текущего среднего и т.п.
        double summ,tmp_double,sigma;

        std::vector<float> data_file_small(countsCanal*(bsaFile.header.nbands+1)); // данные
        std::vector<float> data_file_small_sigma(countsCanal*(bsaFile.header.nbands+1));
                       // стандартное отклонение
        std::vector<float> data_file_sigma_tmp_for_bands(bsaFile.header.nbands+1);
        float limit=1.8;
         //Предел отклонения от медианной сигмы в полосах, выше которого данные отбрасываем
        std::vector<int> data_file_filter(countsCanal*(bsaFile.header.nbands+1));
         //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос

        if (otladka==1)
           {
        OutLine_1="Pnt004 Перед калибровкой n_single_points=" + QString::number(n_single_points);
        ui->LineEdit->setText(OutLine_1);
        //errorMessage.showMessage(OutLine_1);
        //errorMessage.exec();
        };



        if ((otladka==1) )
         {
          writeStream << "\n CHECK BASE DATE before calb. bsaFile.data_file[0],  bsaFile.data_file[bsaFile.data_file.size()-1], size : "
                    << bsaFile.data_file[0] << ", " << bsaFile.data_file[bsaFile.data_file.size()-1] << ", "
                    << bsaFile.data_file.size() << "\n\n"; //Запишет в файл параметры
         };

        //ФАЗА 2: откалибруем данные
        prev_stop_time_program= QDateTime::currentDateTime();

        if ((otladka==1) )
         {
            writeStream << "PERED KALIBROVKOJ size_point,bsaFile.header.npoints,bsaFile.header.nbands,tresolution= "
                        << size_point << ", " << bsaFile.header.npoints << ", " << bsaFile.header.nbands << ", "
                        << tresolution <<  "\n"; //Запишет в файл параметры
          };//if (otladka==1)

        if (otladka==1)
           {
 OutLine_1="Перед калибровкой == " + bsaFile.name_file_of_datas;
        ui->LineEdit->setText(OutLine_1);
        //errorMessage.showMessage(OutLine_1);
        //errorMessage.exec();
        };

        //Запись в конец файла:

        log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
        log_out  << bsaFile.name_file_of_datas.toStdString()  << " : before calb. \n" ;      // сама запись
        //проверка, какая версия OpenMP используется
        #ifdef _OPENMP
        log_out  <<  "OpenMP Version: " << _OPENMP / 100 << " (" << _OPENMP % 100 << ")"  << "\n" ;
        #else
        log_out  << "Sequential Version, not OpenMP \n";
        #endif


        stop_time_program = QDateTime::currentDateTime();
        //float counter_sec_from_start=stop_time_program.secsTo(stop_time_program)-stop_time_program.secsTo(start_time_program);
        counter_ms_from_start=stop_time_program.msecsTo(stop_time_program)-stop_time_program.msecsTo(start_time_program);
        log_out  << "Work_TIME_in_millisec=" << counter_ms_from_start << " millisec.\n" ;

        log_out.close();      // закрываем файл


        // #pragma omp parallel for
        //#pragma omp parallel
        //omp_set_num_threads(8);
        //#pragma omp parallel for num_threads(8)
        //omp_set_num_threads(48);
        //#pragma omp parallel for
        for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)
         //Проход по лучам БСА с целью калибровки
          {
            for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)
              {//Одну за другой калибруем КАЖДую ПОЛОСу, в том числе - ОБЩУЮ
                total=0;

                //Распараллелим через цикл по i_ind ... bsaFile.header.npoints
                #pragma omp parallel for
                for (unsigned int i_ind=0; i_ind < bsaFile.header.npoints; ++i_ind)
                  //Проход по временнЫм точкам
                  {

                    if ((otladka==2))  //&& (counter_otladka>=292))
                     {
                      log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
                      log_out  << bsaFile.name_file_of_datas.toStdString()  << " : Kalibruem: \n" ;
                      //сама запись
                      log_out  << " i_ind=" << i_ind;
                      log_out  << ", bsaFile.header.npoints=" << bsaFile.header.npoints;      // сама запись
                      log_out  << " \n";
                      log_out.close();      // закрываем файл
                     };

                    bsaFile.data_file[size_point * i_ind +
                            currentCanal * (bsaFile.header.nbands+1)+ currentBand] /* calib_data_T */=
                            calib(bsaFile.data_file[size_point * i_ind //(i+start_point)
                                       + currentCanal * (bsaFile.header.nbands+1)
                                       + currentBand],//
                                       tresolution * i_ind, currentBand, currentCanal, otladka_add_1);
                      //Откалибровали к градусам Кельвина значение данных
                    /* При этом:
                     * double grav::calib(double y, double ss, int numberBand, int numberCanal, int otladka_add_1)
                       //Здесь производлится калибровка данных для указанной точки данных
                       И:    double cur_mjd_start = mjd_start + ss / 86400;
                               (причем ss (внутри) = tresolution * i_ind (извне) - это секунды от начала данного отрезка)
                       Где
                       double mjd_start, curTime;
                       //Глобальные переменные для хранения MJD и текущего времени - начала рисунка - из файла gamma.h
                     * - они должны быть рассчитаны выше
                     */


                    if ((otladka==2) && (i_ind % param_1_otladka==0))
                     {
                        writeStream << "size_point,ray,band,i, Y, calb_Y= "
                                    << size_point << ", " << currentCanal << ", " << currentBand << ", "  << i_ind << ", "
                                    << bsaFile.data_file[size_point * i_ind + currentCanal * (bsaFile.header.nbands+1)+ currentBand]
                                    << ", " << calib_data_T << "\n"; //Запишет в файл параметры
                      };//if (otladka==1)

                   // bsaFile.data_file[size_point * i_ind + currentCanal * (bsaFile.header.nbands+1)+ currentBand]=calib_data_T;
                    //Не делаем ничего через промежуточную переменную calib_data_T, иначе в параллельной секции
                    //возникнет произвольная путаница значений!

                  };
                    //Проход по временнЫм точкам
                    //for (unsigned int i_ind=0; i_ind < bsaFile.header.npoints; ++i_ind)

                total=total/bsaFile.header.npoints;
                //Определили среднее вдоль канала на данной полосе, теперь определим дисперсию
                //- может понадобиться в дальнейшем

                summ=0;
                for (unsigned int i_ind=0; i_ind < bsaFile.header.npoints; ++i_ind)
                  //Проход по временнЫм точкам - находим дисперсию, точнее, Среднеквадратическое отклонение
                  //https://ru.wikipedia.org/wiki/%D0%A1%D1%80%D0%B5%D0%B4%D0%BD%D0%B5%D0%BA%D0%B2%D0%B0%D0%B4%D1%80%D0%B0%D1%82%D0%B8%D1%87%D0%B5%D1%81%D0%BA%D0%BE%D0%B5_%D0%BE%D1%82%D0%BA%D0%BB%D0%BE%D0%BD%D0%B5%D0%BD%D0%B8%D0%B5
                  {
                    tmp_double=bsaFile.data_file[size_point * i_ind //(i+start_point)
                            + currentCanal * (bsaFile.header.nbands+1)
                            + currentBand]-total;
                    summ=summ+ tmp_double*tmp_double;
                  };
                    //Проход по временнЫм точкам
                    //for (unsigned int i_ind=0; i_ind < bsaFile.header.npoints; ++i_ind)
                sigma=sqrt(summ/bsaFile.header.npoints);
                data_file_small[currentCanal*(bsaFile.header.nbands+1)+currentBand]=total;
                data_file_small_sigma[currentCanal*(bsaFile.header.nbands+1)+currentBand]=sigma;
                data_file_sigma_tmp_for_bands[currentBand]=sigma;

                if (otladka==2)
                 {
                    writeStream << "Cnl, Band, Calb_Y_sred, Sigma= "
                       << currentCanal << ", " << currentBand
                       << ", " << data_file_small[currentCanal*(bsaFile.header.nbands+1)+currentBand]
                       << ", " << data_file_small_sigma[currentCanal*(bsaFile.header.nbands+1)+currentBand] << "\n";
                         //Запишет в файл параметры
                  };//if (otladka==1)
              };
            //for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)
            //Закончили калибровку КАЖДОЙ ПОЛОСЫ данного луча (в том числе - ОБЩЕЙ)
            //Займемся теперь маскированием плохих частот для данного канала (где сигма слишком отклоняется от
            //среднемедианного вдоль всех частот, брать не будем)


            //Проверим качество данных по отдельным полосам частот. Плохие частотные полосы будем помечать.
            std::sort(data_file_sigma_tmp_for_bands.begin(), data_file_sigma_tmp_for_bands.end());
            epsilon = (data_file_sigma_tmp_for_bands.size()%2 == 0) ?
                        data_file_sigma_tmp_for_bands[data_file_sigma_tmp_for_bands.size()/2]
                    : (data_file_sigma_tmp_for_bands[data_file_sigma_tmp_for_bands.size()/2]
                    + data_file_sigma_tmp_for_bands[data_file_sigma_tmp_for_bands.size()/2 - 1])/2;

            for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)
              {//ДЛЯ КАЖДОЙ ПОЛОСЫ данного луча проверяем уклонение от обычной сигмы
                if (data_file_sigma_tmp_for_bands[currentBand]>limit*epsilon)
                  {data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]=0;}
                else
                  {data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]=1;};
                //Назначили маску =0 - не брать, =1 - брать

                //Теперь пере-устанавливаем стартовый и финишный канал по частоте,
                //которые будем использовать для поиска
                //ВНИМАНИЕ! ЭТО НУЖНО делать для каждого луча отдельно - нужно это делать дальше,
                //перед обработкой каждого луча в отдельности!
                if ((startBand_process<0) &&
                        (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1))
                    startBand_process=currentBand;
                if ((finishBand_process<currentBand) &&
                        (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1))
                    finishBand_process=currentBand;
              };
              //for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)

          };
        //Проход по лучам БСА for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)

        if ((otladka==1) )
         {
          writeStream << "\n CHECK BASE DATE AFTER calb. bsaFile.data_file[0],  bsaFile.data_file[bsaFile.data_file.size()-1], size : "
                    << bsaFile.data_file[0] << ", " << bsaFile.data_file[bsaFile.data_file.size()-1] << ", "
                    << bsaFile.data_file.size() << "\n\n"; //Запишет в файл параметры
         };



        //Запись в конец файла:
        log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
        log_out  << bsaFile.name_file_of_datas.toStdString()  << " : AFTER calb. \n" ;      // сама запись

        stop_time_program = QDateTime::currentDateTime();
        //float counter_sec_from_start=stop_time_program.secsTo(stop_time_program)-stop_time_program.secsTo(start_time_program);
        counter_ms_from_start=stop_time_program.msecsTo(stop_time_program)-stop_time_program.msecsTo(start_time_program);
        log_out  << "Work_TIME_in_millisec=" << counter_ms_from_start << " millisec.\n" ;


        log_out.close();      // закрываем файл


        if (otladka==1)
           {
        OutLine_1="Pnt005 После калибровки countsCanal=" + QString::number(countsCanal);
        ui->LineEdit->setText(OutLine_1);
        //errorMessage.showMessage(OutLine_1);
        //errorMessage.exec();
        };

        if (otladka==2)
          {
             stop_time_program = QDateTime::currentDateTime();
             writeStream << "\n CALIBRATION AND FIND GOOD BAND DATA DID at "
                         << QString::number(stop_time_program.msecsTo(start_time_program))
                         << " msec from start and " //Запишет в файл параметры
                         << QString::number(stop_time_program.msecsTo(prev_stop_time_program))
                         << " msec from previous proc.\n"; //Запишет в файл параметры
          };//if (otladka==1)

        tmp_Canal=4;
        if ((otladka==2) && (tmp_Canal=4))
         {
            tmp_ind=0;
            tmp_band=31;
            writeStream << "CHECK after calb:  Cnl, Band= "
               << tmp_Canal << ", " << tmp_band << "\n";
                 //Запишет в файл параметры
            tmp_max=60*80;
            if (tmp_max>bsaFile.header.npoints-1)
              tmp_max=bsaFile.header.npoints-1;
            while (tmp_ind<=tmp_max)
              {
                writeStream << tmp_ind << ": "
                     << bsaFile.data_file[size_point * tmp_ind +
                      tmp_Canal * (bsaFile.header.nbands+1)+ tmp_band] << ", " ;
                tmp_ind=tmp_ind+80;
              };
            writeStream << "\n";
          };//if (otladka==1)

        prev_stop_time_program= QDateTime::currentDateTime();

        /*
        // Вот код, может Вам пригодится, с использованием STL и лямбда-функций С++11,
        // который извлекает из целого загруженного массива данных по массив данных
        //выбранному каналу и частоте.
        //Формируем вектор по выбранным каналу curCanal и частоте j
        int j = 20;
        std::vector<double> test_v(bsaFile.data_file.size() /
                       ((bsaFile.header.nbands+1) * bsaFile.nCanal * bsaFile.header.modulus.size()) );
        std::copy_if(bsaFile.data_file.begin(), bsaFile.data_file.end(), test_v.begin(),
            [&bsaFile, curCanal, j, nCountPoint](float &d)
              { return ((&d - &bsaFile.data_file[0]) - curCanal * (bsaFile.header.nbands+1) - j)
                %nCountPoint == 0; } );

        //Аналогично можно извлекать из целого массива данные целиком по всему каналу по всем частотам.

        int nCountPoint = (bsaFile.header.nbands+1) * bsaFile.nCanal * bsaFile.header.modulus.size();
           // количество лучей во всех модулях
        std::vector<double> test_v(bsaFile.data_file.size() / (bsaFile.nCanal * bsaFile.header.modulus.size()) );
        std::copy_if(bsaFile.data_file.begin(), bsaFile.data_file.end(), test_v.begin(),
             [&bsaFile, curCanal, nCountPoint](float &d){ return ((&d - &bsaFile.data_file[0])
                - curCanal * (bsaFile.header.nbands+1))%nCountPoint >= 0
              && ((&d - &bsaFile.data_file[0]) - curCanal * (bsaFile.header.nbands+1))
              %nCountPoint < bsaFile.header.nbands; } );

        Заполнение вектора без предварительной задачи его размера.

        std::vector<double> test_v;
        std::copy(arr_counts.begin(), arr_counts.end(), std::back_inserter(test_v));

        Максим

        Алгоритм для среднемедианной, лучше задать размер вектора-приёмника заранее,
        чтобы каждый раз не было накладных расходов на увеличение размера вектора, но в данном
        куске кода просто тестировал заполнение пустого вектора:

        std::vector<double> test_v;
        std::copy(arr_counts.begin(), arr_counts.end(), std::back_inserter(test_v));
        std::sort(test_v.begin(), test_v.end());
        double epsilon = (test_v.size()%2 == 0) ? test_v[test_v.size()/2]
                : (test_v[test_v.size()/2] + test_v[test_v.size()/2 - 1])/2;

        */

            //double y_average = total / (koeff * bsaFile.header.npoints);

        counter_median_point_in_deriv=0;
        //Считаем количество точек, в которых внутри отрезка будем считать среднемедианные значения (раз в 20 сек)
        //От него будем отталкиваться при создании векторов нужного размера, куда будем помещать
        //среднемедианные значения, из интерполяции которых будем создавать линию среднемедианных значений
        //tmp_long=int((1000/bsaFile.header.tresolution) *
        //                  (step_for_median_data_in_sec/2)+0.5);

        tmp_double=(1000/bsaFile.header.tresolution) *
                          (step_for_median_data_in_sec/2);

        /* - закоммментарил, а потом вообще стер код, поскольку дает  лишнюю точку на выходе - проще взять и поделить на 20 сек,
         * а граничные считать отдельно
         */

        counter_median_point_in_deriv=int(bsaFile.header.npoints / ind_in_step_for_median_data);
        //Для 4 минут с "хвостом" из 31 секунды (учет до Д=2000) это - 13 точек

        if (otladka==1)
         {
          writeStream << "FINDING npoints, tmp_double, counter_med ="
                    << bsaFile.header.npoints << ", " << tmp_double << ", "
                    << counter_median_point_in_deriv << "\n"; //Запишет в файл параметры
         };

        //ФАЗА 3: определим средне-медианные данные, дисперсии и т.п.

        //Теперь перейдем к определению среднемедианных значений, чтобы потом их вычесть из уже откалиброванных
        std::vector<float> media_vec(counter_median_point_in_deriv*countsCanal*(bsaFile.header.nbands+1));
         //Вектор из медианных значений вдоль countsCanal лучей и bsaFile.header.nbands полос .
         //С отрезками по 20 секунд - выше нашли, сколько точек: counter_median_point_in_deriv

        std::vector<float> med_vec_start(countsCanal*(bsaFile.header.nbands+1));
         //НАЧАЛЬНЫЙ (первые 5 секунд) Вектор из медианных значений для countsCanal лучей и bsaFile.header.nbands полос .
         //Нужен для устранения граничных эффектов вычитания среднемедианного из данных

        std::vector<float> med_vec_finish(countsCanal*(bsaFile.header.nbands+1));
         //КОНЕЧНЫЙ (последние 5 секунд) Вектор из медианных значений для countsCanal лучей и bsaFile.header.nbands полос .
         //Нужен для устранения граничных эффектов вычитания среднемедианного из данных

        std::vector<float> min_data_vec(counter_median_point_in_deriv*countsCanal*(bsaFile.header.nbands+1));
         //Вектор из минимальных значений раз в step_for_median_data_in_sec секунд вдоль countsCanal лучей и bsaFile.header.nbands полос .
        std::vector<float> max_data_vec(counter_median_point_in_deriv*countsCanal*(bsaFile.header.nbands+1));
         //Вектор из максимальных значений раз в step_for_median_data_in_sec секунд вдоль countsCanal лучей и bsaFile.header.nbands полос .
        std::vector<float> sigma_data_vec(counter_median_point_in_deriv*countsCanal*(bsaFile.header.nbands+1));
         //Вектор из значений дисперсий (отклонений от среднего) раз в step_for_median_data_in_sec секунд  вдоль countsCanal лучей и bsaFile.header.nbands полос .
        std::vector<long> max_data_vec_index(counter_median_point_in_deriv*countsCanal*(bsaFile.header.nbands+1));
         //Вектор из ИНДЕКСОВ максимальных значений внутри текущего часа данных раз в step_for_median_data_in_sec секунд вдоль countsCanal лучей и bsaFile.header.nbands полос .

        std::vector<float> test_vec(bsaFile.header.npoints);
        //Вектор, куда будем помещать точки с одного из лучей, одной из полос
        //- полностью, от начального момента времени до конечного

        long ind_beg,ind_center,ind_end;
        ind_beg=int(ind_in_step_for_median_data*0.04 +0.5);
        //Отбрасываем нижние 4% точек - там бывают редкие сбои, смещающие среднее
        ind_end=int(ind_in_step_for_median_data-ind_in_step_for_median_data*0.30 +0.5);
        //Отбрасываем верхние 30% точек - там бывают всплески (в том числе - длинные), смещающие среднее значение на отрезке данных
        ind_center=ind_beg+(ind_end-ind_beg)/2;

        if (otladka==2)
         {
          writeStream << "\n TESTING INDEXes npoints,step_for_median_data_in_sec,ind_in_step_for_median_data,counter_median_point_in_deriv; ind_beg,ind_center,ind_end == "
                    << bsaFile.header.npoints << ", \n" << step_for_median_data_in_sec << ", "
                    << ind_in_step_for_median_data << ", "
                    << counter_median_point_in_deriv << "; "
                    << ind_beg << ", " << ind_center << ", " << ind_end << ", "
                    << "\n"; //Запишет в файл параметры
         };

        if (otladka==2)
           {
        OutLine_1="Pnt006 Набрали начальные вектора для сортировки";
        ui->LineEdit->setText(OutLine_1);
        errorMessage.showMessage(OutLine_1);
        errorMessage.exec();
        };

        if (otladka==1)
           {
 OutLine_1="Перед сортировкой == " + bsaFile.name_file_of_datas;
        ui->LineEdit->setText(OutLine_1);
        //errorMessage.showMessage(OutLine_1);
        //errorMessage.exec();
        };
        for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)
            //Проход по лучам БСА currentCanal - ищем среднемедианные значения
          {
            for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)
              {//Проход по ВСЕМ полосам (в том числе - ОБЩЕЙ) - ищем среднемедианные значения

                if ((otladka==1) && (currentCanal == 0) && (currentBand == startBand))
                 {
                  writeStream << "\n PERED READ v vektor! ray,band,npoints,ind_in_step, counter_med - find eps: "
                            << currentCanal << ", " << currentBand << ", "
                            << bsaFile.header.npoints << ", " << ind_in_step_for_median_data << ", "
                            << counter_median_point_in_deriv << "\n"; //Запишет в файл параметры
                 };

                //Вначале считаем из общего многомерного вектора (считанного и откалиброванного куска файла)
                //вектор всех данных в конкретной полосе currentBand:
                for (unsigned int i_ind=0; i_ind < bsaFile.header.npoints; ++i_ind)
                   //Проход по временнЫм точкам
                   {
                      test_vec[i_ind]=bsaFile.data_file[size_point * i_ind
                                                       + currentCanal * (bsaFile.header.nbands+1)
                                                       + currentBand];
                   };

                if ((otladka==1) && (currentCanal == 0) && (currentBand == startBand))
                 {
                  writeStream << "\n TEST VEC for cnl=0,band=0; test_vec[0], test_vec[npoints-1], npoints ,currentBand : "
                            << test_vec[0] << ", " << test_vec[bsaFile.header.npoints-1] << ", "
                            << bsaFile.header.npoints << ", " << currentBand << ", ind_in_step_for_median_data="
                             << ind_in_step_for_median_data << ", counter_median_point_in_deriv="
                            << counter_median_point_in_deriv << "\n"; //Запишет в файл параметры
                 };
                //nCountPoint единичных точек в одной временнОй точке
                //int size_point = (bsaFile.header.nbands+1) * bsaFile.nCanal * bsaFile.header.modulus.size();
                //Число данных файла в 1 временной точке, для 6 полос и 48 лучей =336 - т.е. ТО ЖЕ САМОЕ

                /* Нижеследующая конструкция дала ошибку:
                   std::copy_if(bsaFile.data_file.begin(), bsaFile.data_file.end(), test_vec.begin(),
                               [&bsaFile, currentCanal, currentBand, nCountPoint](float &d)
                               { return ((&d - &bsaFile.data_file[0]) - currentCanal * (bsaFile.header.nbands+1) - currentBand)
                                        %nCountPoint == 0; } );
                                  D:\DAT_SAM\SURVEY\NEW\Program\Toropov_Maxim\Pokanalno\Last_05\gamma.cpp:2062:
                                   ошибка: capture of non-variable 'grav::bsaFile'
                                  [&bsaFile, currentCanal, currentBand, nCountPoint](float &d)
                                    ^
                                  D:\DAT_SAM\SURVEY\NEW\Program\Toropov_Maxim\Pokanalno\Last_05\gamma.cpp:2063:
                                    ошибка: 'this' was not captured for this lambda function
                                       { return ((&d - &bsaFile.data_file[0]) - currentCanal * (bsaFile.header.nbands+1) - currentBand)
                                                                                                ^
                */

                for (int cur_median_point = 0;
                                  cur_median_point < counter_median_point_in_deriv; cur_median_point++)
                  {//Проход по сетке, где ищутся медианные значения cur_median_point - растет вдоль времени
                    long start_ind= int((cur_median_point*step_for_median_data_in_sec /
                                                        (0.001 * bsaFile.header.tresolution)) +0.5);
                    long finish_ind=start_ind+ind_in_step_for_median_data;

                    if ((otladka==1) && (currentCanal == 0) && (currentBand == startBand))
                     {
                      writeStream << "\n TEST ind for VEC for cnl=0,band=0; start_ind,ind_in_step_for_median_data,finish_ind,npoints : "
                                << start_ind << ", " << ind_in_step_for_median_data << ", "
                                << finish_ind << ", counter_median_point_in_deriv="
                                << bsaFile.header.npoints
                                << "\n"; //Запишет в файл параметры
                     };

                    while (finish_ind>bsaFile.header.npoints)
                      {//На всякий случай проконтролируем невыход за границы всего вектора
                         start_ind=start_ind-1;
                         finish_ind=finish_ind-1;
                      };

                    std::vector<float> for_sort_vec(test_vec.begin() + start_ind, test_vec.begin() + finish_ind);
                     //Скопировали кусок вектора длиной ind_in_step_for_median_data - для последующей сортировки
                    if ((otladka==1) && (currentCanal == 0) && (currentBand == startBand))
                     {
                      writeStream << "\n COPY PART VEC for cnl=0,band=0; for_sort_vec.at(0),for_sort_vec.at(finish_ind-start_ind) : "
                                << for_sort_vec.at(0) << ", " //<< for_sort_vec.at(finish_ind-start_ind)
                                << "\n"; //Запишет в файл параметры
                     };

                    std::sort(for_sort_vec.begin(), for_sort_vec.end());
                     //отсортировали вектор

                    epsilon = for_sort_vec[ind_center];
                         //- это среднемедианное значение, взли из центра отсортированного вектора данных
                    //media_vec(counter_median_point_in_deriv*countsCanal*(bsaFile.header.nbands+1));
                    media_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                              +currentBand*counter_median_point_in_deriv+cur_median_point]=epsilon;


                    min_data_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                          +currentBand*counter_median_point_in_deriv+cur_median_point]=for_sort_vec[0];
                      //Вектор из минимальных значений раз в step_for_median_data_in_sec секунд

                    max_data_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                          +currentBand*counter_median_point_in_deriv+cur_median_point]=for_sort_vec[for_sort_vec.size()-1];
                      //Вектор из максимальных значений раз в step_for_median_data_in_sec секунд


                     //ind_center=ind_beg+(ind_end-ind_beg)/2;

                    summ=0;
                    for (int i_ind=ind_beg; i_ind <=ind_end; ++i_ind)
                       //Проход по временнЫм точкам для поиска дисперсии
                      {
                        tmp_double=for_sort_vec[i_ind]-epsilon;
                        summ=summ+ tmp_double*tmp_double;
                      };
                      //Проход по временнЫм точкам
                      //for (unsigned int i_ind=ind_beg; i_ind <=ind_end; ++i_ind)
                  tmp_double=ind_end-ind_beg; //т.е. n-1 значений
                  sigma=sqrt(summ/tmp_double);
                  //Из https://ru.wikipedia.org/wiki/%D0%A1%D1%80%D0%B5%D0%B4%D0%BD%D0%B5%D0%BA%D0%B2%D0%B0%D0%B4%D1%80%D0%B0%D1%82%D0%B8%D1%87%D0%B5%D1%81%D0%BA%D0%BE%D0%B5_%D0%BE%D1%82%D0%BA%D0%BB%D0%BE%D0%BD%D0%B5%D0%BD%D0%B8%D0%B5
                  // Стандартное отклонение (оценка среднеквадратического отклонения случайной величины
                  // x относительно её математического ожидания на основе несмещённой оценки её дисперсии)  s:
                  sigma_data_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                          +currentBand*counter_median_point_in_deriv+cur_median_point]=sigma;
                             //Вектор из значений дисперсий (отклонений от среднего) раз в step_for_median_data_in_sec
                             //секунд  вдоль countsCanal лучей и bsaFile.header.nbands полос .

                  //Определим номер индекса максимального значения на данном отрезке - потом мы его запишем в файл.
                  int ind_max=start_ind; //взяли для старта самое минимальное...
                  for (int i_ind=start_ind; i_ind <finish_ind; i_ind++)
                     //Проход по временнЫм точкам для поиска расположения максимального значения
                    {
                     if(test_vec[ind_max]<test_vec[i_ind])
                        ind_max=i_ind;
                    };
                  max_data_vec_index[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                          +currentBand*counter_median_point_in_deriv+cur_median_point]=ind_max+start_point ;
                    //start_point - именно на столько точек мы сдвинулись от начала файла при чтении данных из него



                  if ((otladka==2) && (currentCanal==4))
                    {
                       writeStream << "i_ind_med_tot, i_1, i_2, ind_beg, ind_center, ind_end, MEDIAN, min, max, Ind_MAX, TEST_max, sigma= "
                                   << cur_median_point << ", " << start_ind << ", " << finish_ind << ", "
                                   << ind_beg << ", " << ind_center << ", " << ind_end << ", "
                                   << media_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                                       +currentBand*counter_median_point_in_deriv+cur_median_point] << ", "
                                   << min_data_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                                       +currentBand*counter_median_point_in_deriv+cur_median_point] << ", "
                                   << max_data_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                                       +currentBand*counter_median_point_in_deriv+cur_median_point] << ", "
                                   << max_data_vec_index[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                                       +currentBand*counter_median_point_in_deriv+cur_median_point] << ", "
                                   <<   test_vec[ind_max] << " ! "
                                   << sigma_data_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                                       +currentBand*counter_median_point_in_deriv+cur_median_point] << ", "
                                   << "\n"; //Запишет в файл параметры
                    };//if (otladka==1)

                  };//for (unsigned int cur_median_point = 0;
                    //cur_median_point < counter_median_point_in_deriv; cur_median_point++)

                //Определим граничные медианные по первым и последним 5 секундам
                unsigned int ind_start=0;
                unsigned int ind_stop= int(5.0/(0.001 * bsaFile.header.tresolution)+0.5);
                std::vector<float> for_sort_vec(test_vec.begin() + ind_start, test_vec.begin() + ind_stop);
                 //Скопировали кусок вектора длиной ind_in_step_for_median_data

                std::sort(for_sort_vec.begin(), for_sort_vec.end());
                unsigned int ind_center_tmp=int(ind_stop*0.04+ (ind_stop-ind_stop*0.30 - ind_stop*0.04)/2.0 +0.5);
                epsilon = for_sort_vec[ind_center_tmp];

                med_vec_start[currentCanal*(bsaFile.header.nbands+1)+currentBand]=epsilon;
                         //НАЧАЛЬНЫЙ (первые 5 секунд) Вектор из медианных значений для countsCanal лучей и bsaFile.header.nbands полос .
                         //Нужен для устранения граничных эффектов вычитания среднемедианного из данных

                if ((otladka==2) && (currentCanal==4))
                 {
                    writeStream  << "\n Vec_start: Canal, Band, ind_start, ind_stop, ind_center_tmp, med_vec_start  "
                            << currentCanal << ", " << currentBand << ", "
                            << ind_start << ", " << ind_stop << ", " << ind_center_tmp << ", "
                            << med_vec_start[currentCanal*(bsaFile.header.nbands+1)+currentBand] << "\n"; //Запишет в файл параметры
                 };//if (otladka==1)

                ind_start=bsaFile.header.npoints-ind_stop;
                ind_stop=bsaFile.header.npoints;
                std::vector<float> for_sort_vec_2(test_vec.begin() + ind_start, test_vec.begin() + ind_stop);
                std::sort(for_sort_vec_2.begin(), for_sort_vec_2.end());
                epsilon = for_sort_vec_2[ind_center_tmp];
                med_vec_finish[currentCanal*(bsaFile.header.nbands+1)+currentBand]=epsilon;

                if ((otladka==2) && (currentCanal==4))
                 {
                    writeStream  << "\n Vec_END: Canal, Band, ind_start, ind_stop, ind_center_tmp, med_vec_start  "
                                 << currentCanal << ", " << currentBand << ", "
                                 << ind_start << ", " << ind_stop << ", " << ind_center_tmp << ", "
                            << med_vec_finish[currentCanal*(bsaFile.header.nbands+1)+currentBand] << "\n"; //Запишет в файл параметры
                 };//if (otladka==1)

              };
              //for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)

          };
           //Проход по лучам БСА for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)

        //Запись в конец файла:
        //std::ofstream log_out;                    //создаем поток
        log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
        log_out  << bsaFile.name_file_of_datas.toStdString()  << " : after sorting. \n" ;      // сама запись

        stop_time_program = QDateTime::currentDateTime();
        counter_ms_from_start=stop_time_program.msecsTo(stop_time_program)-stop_time_program.msecsTo(start_time_program);
        log_out  << "Work_TIME_in_millisec=" << counter_ms_from_start << " millisec.\n" ;

        log_out.close();      // закрываем файл

        if (otladka==1)
           {
 OutLine_1="ПОСЛЕ сортировки == " + bsaFile.name_file_of_datas;
        ui->LineEdit->setText(OutLine_1);
        //errorMessage.showMessage(OutLine_1);
        //errorMessage.exec();
        };
        if (otladka==1)
           {
        OutLine_1="Pnt007 Закончили сортировку";
        ui->LineEdit->setText(OutLine_1);
        //errorMessage.showMessage(OutLine_1);
        //errorMessage.exec();
        };

        if (otladka==1)
          {
             stop_time_program = QDateTime::currentDateTime();
             writeStream << "\n FIND MEDIAN DATA DID at "
                         << QString::number(stop_time_program.msecsTo(start_time_program))
                         << " msec from start and " //Запишет в файл параметры
                         << QString::number(stop_time_program.msecsTo(prev_stop_time_program))
                         << " msec from previous proc.\n"; //Запишет в файл параметры
          };//if (otladka==1)


        if (otladka==2)
          {
             writeStream << "PERED MINUS median ray=0, band=0, ind=0: MEDIAN, DATA, counter_median_point_in_deriv= "
                         << media_vec[0] << ", " << bsaFile.data_file[0]
                         << "\n"; //Запишет в файл параметры
             writeStream << "PERED MINUS median ray=47,  ind=npoints-1: BAND: MEDIAN, DATA , counter_median_point_in_deriv= "
                         << finishBand << ": "
                         << media_vec[47*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                            +finishBand*counter_median_point_in_deriv+counter_median_point_in_deriv-1]
                         << ", " << bsaFile.data_file[bsaFile.data_file[size_point*(bsaFile.header.npoints-1) +
                                 47*(bsaFile.header.nbands+1) + finishBand]]
                         << "\n"; //Запишет в файл параметры
          };//if (otladka==1)

        prev_stop_time_program= QDateTime::currentDateTime();

        tmp_Canal=4;
        if ((otladka==2) && (tmp_Canal==4))
         {
            tmp_ind=0;
            tmp_band=31;
            writeStream << "CHECK setka MEDIAN-point:  Cnl, start_med, end_med= "
               << tmp_Canal << ", " << tmp_band
               << med_vec_start[tmp_Canal*(bsaFile.header.nbands+1)+tmp_band] << ", "
               << med_vec_finish[tmp_Canal*(bsaFile.header.nbands+1)+tmp_band] << "\n";
                 //Запишет в файл параметры
            tmp_max=counter_median_point_in_deriv-1;
            while (tmp_ind<=tmp_max)
              {
                writeStream << tmp_ind << ": "
                     << media_vec[tmp_Canal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                        +tmp_band*counter_median_point_in_deriv+tmp_ind] << ", " ;
                tmp_ind=tmp_ind+1;
              };
            writeStream << "\n";
          };//if (otladka==1)

        //Теперь вычтем среднемедианные значения (интерполированием вектора-сетки медианных значений)
        //Из откалиброванных значений
        for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)
            //Проход по лучам БСА
          {
            for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)
              {//Проход по полосам

                //Вначале вычтем начало (во времени) общего массива - без интерполяции , просто вычитая первую точку
                //Краевые эффекты, вероятно, есть, но ими можно пренебречь
                ind_beg=0;
                ind_end=ind_in_step_for_median_data/2 +1;
                   //int( step_for_median_data_in_sec / 2.0 / (0.001 * bsaFile.header.tresolution) )+1;
                for (int i_ind=ind_beg; i_ind <= ind_end; ++i_ind)
                  //Проход по временнЫм точкам, вычитаем среднемедианное (первое)
                 {
                    x1 = 0;
                    x2 = ind_end;
                    y1 = med_vec_start[currentCanal*(bsaFile.header.nbands+1)+currentBand];
                    //НАЧАЛЬНЫЙ (первые 5 секунд) Вектор из медианных значений для countsCanal лучей и bsaFile.header.nbands полос .
                    y2 = media_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                            +currentBand*counter_median_point_in_deriv+0];
                    calib_data_T=y1 + (y2 - y1) * (i_ind - x1) / (x2 - x1) ;
                     //Интерполяция медианного на текущий индекс от опорных значений
                    bsaFile.data_file[size_point*i_ind + currentCanal*(bsaFile.header.nbands+1) + currentBand]=
                            bsaFile.data_file[size_point*i_ind + currentCanal*(bsaFile.header.nbands+1) + currentBand]
                           - calib_data_T;

                    /*
                  calib_data_T=bsaFile.data_file[size_point*i_ind + currentCanal*(bsaFile.header.nbands+1) + currentBand]
                     //for (unsigned int cur_median_point = 0;
                     //                       cur_median_point < counter_median_point_in_deriv; cur_median_point++)
                     //  - media_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                     //  +currentBand*counter_median_point_in_deriv+cur_median_point];
                    - media_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                            +currentBand*counter_median_point_in_deriv];
                  bsaFile.data_file[size_point*i_ind + currentCanal*(bsaFile.header.nbands+1) + currentBand]=calib_data_T;
                  */
                 };
                  //вычли среднемедианное (первое)

                for (int cur_median_point = 1;
                              cur_median_point<counter_median_point_in_deriv; cur_median_point++)
                 {
                  //Проходим по сетке точек среднемедианных и интерполируем, вычитаем
                  ind_beg=ind_end+1;
                  ind_end=ind_in_step_for_median_data*cur_median_point+ind_in_step_for_median_data/2+1;

                  x1 = ind_in_step_for_median_data*(cur_median_point-1)+ind_in_step_for_median_data/2;
                  x2 = x1 + ind_in_step_for_median_data;
                  y1 = media_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                          +currentBand*counter_median_point_in_deriv+cur_median_point-1];
                  y2 = media_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                          +currentBand*counter_median_point_in_deriv+cur_median_point];
                  //Нашли все опорные значения

                  //Распараллелим через цикл по i_ind ... ind_end
                  //#pragma omp parallel for
                  for (int i_ind=ind_beg; i_ind <= ind_end; ++i_ind)
                  //Проход по временнЫм точкам, вычитаем среднемедианное (между первым и последним - интерполяция)
                   {

                    calib_data_T=y1 + (y2 - y1) * (i_ind - x1) / (x2 - x1) ;
                     //Интерполяция медианного на текущий индекс от опорных значений
                    bsaFile.data_file[size_point*i_ind + currentCanal*(bsaFile.header.nbands+1) + currentBand]=
                            bsaFile.data_file[size_point*i_ind + currentCanal*(bsaFile.header.nbands+1) + currentBand]
                           - calib_data_T /* (y1 + (y2 - y1) * (i_ind - x1) / (x2 - x1))*/;
                   };
                 };// for (unsigned int cur_median_point = 1;
                   //cur_median_point<=counter_median_point_in_deriv; cur_median_point++)
                //Вычислили все 20-секундные данные , перебирая cur_median_point<counter_median_point_in_deriv
                //- вычли ИНТЕРПОЛИРОВАННЫЕ среднемедианные из откалиброванных данных


                ind_beg=ind_end+1;
                ind_end= bsaFile.header.npoints-1;
                for (int i_ind=ind_beg; i_ind <= ind_end; ++i_ind)
                  //Проход по временнЫм точкам, вычитаем среднемедианное (ПОСЛЕ последнего)
                 {
                    x1 = ind_beg;
                    x2 = ind_end;
                    y1 = media_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                            +currentBand*counter_median_point_in_deriv+counter_median_point_in_deriv-1];
                    y2 = med_vec_finish[currentCanal*(bsaFile.header.nbands+1)+currentBand];
                    //КОНЕЧНЫЙ (последние 5 секунд) Вектор из медианных значений
                    //для countsCanal лучей и bsaFile.header.nbands полос .
                    calib_data_T=y1 + (y2 - y1) * (i_ind - x1) / (x2 - x1) ;
                     //Интерполяция медианного на текущий индекс от опорных значений
                    bsaFile.data_file[size_point*i_ind + currentCanal*(bsaFile.header.nbands+1) + currentBand]=
                            bsaFile.data_file[size_point*i_ind + currentCanal*(bsaFile.header.nbands+1) + currentBand]
                           - calib_data_T;

                 };

                  //вычли среднемедианное (ПОСЛЕ последнего)
              };
              //for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)

          };
           //Проход по лучам БСА for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)

        if (otladka==1)
           {
            OutLine_1="Pnt008 Вычли среднемедианные значения";
            ui->LineEdit->setText(OutLine_1);
            //errorMessage.showMessage(OutLine_1);
            //errorMessage.exec();
           };

        if (otladka==2)
          {
             stop_time_program = QDateTime::currentDateTime();
             writeStream << "\n MINUS MEDIAN DID at "
                         << QString::number(stop_time_program.msecsTo(start_time_program))
                         << " msec from start and " //Запишет в файл параметры
                         << QString::number(stop_time_program.msecsTo(prev_stop_time_program))
                         << " msec from previous proc.\n"; //Запишет в файл параметры
             fileOut.flush();
          };//if (otladka==1)

        tmp_Canal=4;
        if ((otladka==2) && (tmp_Canal=4))
         {
            tmp_ind=0;
            tmp_band=31;
            writeStream << "CHECK after CALB-MEDIAN:  Cnl, Band= "
               << tmp_Canal << ", " << tmp_band << "\n";
                 //Запишет в файл параметры
            //tmp_max=60*80;
            tmp_max=bsaFile.header.npoints-1;
            if (tmp_max>bsaFile.header.npoints-1)
              tmp_max=bsaFile.header.npoints-1;
            while (tmp_ind<=tmp_max)
              {
                writeStream << tmp_ind << ": "
                     << bsaFile.data_file[size_point * tmp_ind +
                      tmp_Canal * (bsaFile.header.nbands+1)+ tmp_band] << ", " ;
                tmp_ind=tmp_ind+80;
              };
            writeStream << "\n";
          };//if (otladka==1)

        if ((otladka==2) && (tmp_Canal=4))
         {
            tmp_band=31;
            writeStream << "CHECK after CALB-MEDIAN, part array:  Cnl, Band= "
               << tmp_Canal << ", " << tmp_band << "\n";
                 //Запишет в файл параметры

            tmp_ind=1410;
            tmp_max=1450;
            while (tmp_ind<=tmp_max)
              {
                writeStream << tmp_ind << ": "
                     << bsaFile.data_file[size_point * tmp_ind +
                      tmp_Canal * (bsaFile.header.nbands+1)+ tmp_band] << ", " ;
                tmp_ind=tmp_ind+1;
              };
            writeStream << "\n";
          };//if (otladka==1)

        if (otladka==2)
          {
             writeStream << "AFTER MINUS median ray=0, band=0, ind=0: MEDIAN, DATA= "
                         << media_vec[0] << ", " << bsaFile.data_file[0]
                         << "\n"; //Запишет в файл параметры
             writeStream << "AFTER MINUS median ray=47,  ind=npoints-1: BAND: MEDIAN, DATA= "
                         << finishBand << ": "
                         << media_vec[47*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                            +finishBand*counter_median_point_in_deriv+counter_median_point_in_deriv-1]
                         << ", " << bsaFile.data_file[bsaFile.data_file[size_point*(bsaFile.header.npoints-1) +
                                 47*(bsaFile.header.nbands+1) + finishBand]]
                         << "\n"; //Запишет в файл параметры
          };//if (otladka==1)

        //std::ofstream log_out;                    //создаем поток
        log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
        log_out  << bsaFile.name_file_of_datas.toStdString()  << " : after minus median. \n" ;      // сама запись
        log_out.close();      // закрываем файл
//======================================================================================================================
        //ФАЗА 4: поиск импульсов с дисперсиями
        //Дисперсионная_Задержка_в_сек=4,15*1000*Dispers*(СТЕПЕНЬ(frec_min;-2) - СТЕПЕНЬ(frec_max;-2))
        //Где frec_min и frec_max - в МГц
        // Дисперсионная_Задержка_в_сек=4,15*1000*Dispers* ((pow(frec_min,-2) - (pow(frec_max,-2))
        //Для 32 полос даже для D=1 , frec_min=109,049, frec_max=111,461
        // --> dt=0,014940476 >0.0125   D=1338   dt=19,99035736   D=2008 dt=30,00047652 (Для D=2000 dt=29,88)
        //Сетка поиска для тонких:

        /*

     float maska[100] = ....

        //Для 6 полос лишь для D=8, frec_min=109,2075, frec_max=111,2876
             dt= 0,103091476
        // --> dt=0,014940476 >0.0125   D=500   dt=7,47
        //Сетка поиска для грубых:
     float maska[70] = { 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0,
                      15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
                      25.0, 26.0, 28.0, 30.0, 32.0, 34.0, 36.0, 38.0, 40.0, 42.0,
                      44.0, 47.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0,
                      90.0, 95.0, 100.0, 110.0, 120.0, 130.0, 140.0, 150.0, 160.0, 170.0,
            180.0, 190.0, 200.0, 210.0, 220.0, 230.0, 240.0, 250.0, 260.0, 280.0,
            300.0, 320.0, 340.0, 360.0, 380.0, 400.0, 430.0, 460.0, 490.0, 520.0
};

     //Векторный объект может быть инициализирован с помощью массива встроенного типа:
     // 100 элементов ia копируются в ivec
    vector< float > dispers_vec( maska, maska+100 );
    //Конструктору вектора ivec передаются два указателя – указатель на начало массива ia и
    //на элемент, следующий за последним.

     */

        //Теперь отрабатываем алгоритм перебора дисперсий

        float control_level, control_level_from_sigma_all_band, limit_sigma_all_band;
        float limit_down; //Готовим переменную для запоминания нижнего предела (чтобы не было 0)
        int counter_good_bands=0;
        long counter_point_in_Dispers_array;
        long counter_point_in_Dispers_array_total=0;
        long counter_point_in_Dispers_array_total_in_ALL_RAYS=0;

        double d_T_in_band,d_X;


        std::vector<float> test_vec_end(bsaFile.header.npoints);
        //сюда будем складывать сумму полос луча в зависимости от дисперсии

        std::vector<float> test_vec_end_tmp(bsaFile.header.npoints);
        //сюда будем складывать сумму полос луча в зависимости от дисперсии - _временную_ МАКСИМАЛЬНУЮ величину

        std::vector<float> myltiply_vec(bsaFile.header.npoints);
        //сюда запоминаем коэффициент превышения, если он больше limit_myltiply (обычно 3.0) и больше уже запомненного
        //для данного индекса myltiply_vec значения

        std::vector<float> dispersion_vec(bsaFile.header.npoints);
        //Сюда будем складывать дисперсии при обновлении myltiply_vec[] - т.е., если нашли более лучшее соотношение
        //для некой дисперсии, то и пере-внесем ее в этот вектор dispersion_vec[] вместе с изменением myltiply_vec[]

        float maska_32[100] = { 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0,
                     12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 22.0,
                     24.0, 26.0, 28.0, 30.0, 33.0, 36.0, 39.0, 42.0, 45.0, 48.0,
                     51.0, 54.0, 57.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0,
                     95.0, 100.0, 105.0, 110.0, 120.0, 130.0, 140.0, 150.0, 160.0, 170.0,
                    180.0, 190.0, 200.0, 210.0, 220.0, 230.0, 240.0, 250.0, 260.0, 270.0,
                    280.0, 300.0, 320.0, 340.0, 360.0, 380.0, 400.0, 420.0, 440.0, 480.0,
                    500.0, 520.0, 540.0, 560.0, 580.0, 600.0, 650.0, 700.0, 750.0, 800.0,
                    850.0, 900.0, 950.0, 1000.0, 1050.0, 1100.0, 1150.0, 1200.0, 1250.0, 1300.0,
                 1350.0, 1400.0, 1450.0, 1500.0, 1550.0, 1600.0, 1700.0, 1800.0, 1900.0, 2000.0};
        //Внимание! Начиная с D>26 - разница во времени между двумя соседними полосами становится больше разрешения 12.5 мс
        //Это граничное значение хранится в limit_round_Disper

        float maska_6[100] = { 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0,
                               15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
                               25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0, 32.0, 33.0, 34.0,
                               36.0, 37.0, 38.0, 39.0, 40.0, 41.0, 42.0, 43.0, 44.0, 45.0,
                               46.0, 47.0, 48.0, 49.0, 50.0, 52.0, 54.0, 56.0, 58.0, 60.0,
                               62.0, 64.0, 66.0, 68.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0,
                     100.0, 110.0, 120.0, 130.0, 140.0, 150.0, 160.0, 170.0, 180.0, 190.0,
                     200.0, 210.0, 220.0, 230.0, 240.0, 250.0, 260.0, 270.0, 280.0, 290.0,
                     300.0, 310.0, 320.0, 330.0, 340.0, 350.0, 360.0, 370.0, 380.0, 390.0,
                     400.0, 420.0, 440.0, 460.0, 480.0, 500.0, 520.0, 540.0, 560.0, 580.0};
        //Внимание! Начиная с D>39 - разница во времени между двумя соседними полосами становится больше разрешения 100 мс
        //Это граничное значение хранится в limit_round_Disper

        //dispersion_vec(maska_32, maska_32+100);

        std::vector<float> dispersion_maska(100);
        float limit_round_Dispers;
        if (bsaFile.header.nbands==32)
         {
          dispersion_maska.assign(maska_32, maska_32+100);
          limit_round_Dispers=26.0;
         }
        else
         {
          dispersion_maska.assign(maska_6, maska_6+100);
          limit_round_Dispers=39.0;
         };
         //Скопировали в вектор маску дисперсий

        long check_npoints = int((1000/bsaFile.header.tresolution) * deriv_star +0.5);
        //В этом интервале точек (начиная с 0) будем складывать полосы со смещением

        long d_ind_x1;
        long d_ind_x2, ind_2;
        float d_X_after_left_ind;
        double srednee_all_band,sigma_all_band;


        if (otladka==1)
           {
            OutLine_1="Pnt009-1 Перед записью в файл значений. device,countsCanal,n_bands,resolution ; date_begin="
                + QString::number(device) + "," + QString::number(countsCanal) + "," + QString::number(n_bands) + ","
                + QString::number(resolution,'f', 4) + " ; " + bsaFile.header.date_begin + " " + bsaFile.header.time_begin;
            ui->LineEdit->setText(OutLine_1);
            //errorMessage.showMessage(OutLine_1);
            //errorMessage.exec();
           };
        //Теперь вначале запишем накопленные среднемедианные (и дисперсионные) данные по полосам и лучам
        //- чтобы потом их использовать для залития в базу данных либо работы в офф-лайне
        //(определять средние месячные шаблоны)
        write_files_short_data_from_calb_data(out_data_dir,name_file_short,device,countsCanal,n_bands,
                                              resolution,
                                              bsaFile.header.date_begin, // Дата начала регистрации файла в UTC, считано ранее из заголовка
                                              bsaFile.header.time_begin, // Время начала регистрации файла в UTC, считано ранее из заголовка
                                              step_in_star_second,//шаг в звездных секундах
                                              star_time_in_Hour_start_file,
                                              //стартовая ЗВЕЗДНАЯ секунда НАЧАЛА часа (файла)
                                              star_time_start_end_H,
                                              //стартовая ЗВЕЗДНАЯ секунда часа,
                                              //с которой все начинается - посчитана для i=0 и не меняется !
                                              start_time_in_sec_end,
                                              //стартовая ВРЕМЕННАЯ секунда часа, с которой все начинается
                                              // - посчитана для i=0 и не меняется !
                                              start_point,
                                              // - именно на столько точек мы сдвинулись от начала файла при чтении данных из него,
                                              // считается при каждом новом i - ЗАНОВО
                                              mjd_start,
                                              counter_median_point_in_deriv,//точек в массивах
                                              i, //Номер считанного КУСКА-порции файла данных
                                              //по counter_median_point_in_deriv
                                              deriv_star,
                                              // количество сдвиговых секунд,вычисленных из ЗВЕЗДНЫХ секунд в порции данных
                                              //- реально их должно быть меньше, чтобы не было разрывов в данных
                                              media_vec,
                                              min_data_vec,
                                              max_data_vec,
                                              sigma_data_vec,
                                              max_data_vec_index);
        //Записали 20-ти секунднЫЕ точкИ данных данного временного отрезка - среднемедианные, дисперсии и т.д.

        //Запись в конец файла-лога:
        //std::ofstream log_out;                    //создаем поток
        log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
        log_out  << bsaFile.name_file_of_datas.toStdString()  << " : after writing median.\n" ;      // сама запись

        stop_time_program = QDateTime::currentDateTime();
        counter_ms_from_start=stop_time_program.msecsTo(stop_time_program)-stop_time_program.msecsTo(start_time_program);
        log_out  << "Work_TIME_in_millisec=" << counter_ms_from_start << " millisec.\n" ;

        log_out.close();      // закрываем файл

        if (otladka==2)
                  {
                     //str1 = now.toString;
                     writeStream << "\n POSLE write_files_short_data_from_calb_data device,countsCanal,n_bands,resolution,="
                     << device << "," ; //Запишет в файл параметры
                     writeStream << countsCanal << " ,"
                                 << n_bands << " ,"
                                 << resolution << " step_in_star_second,star_time_in_Hour_start_file,star_time_start_end_H,start_time_in_sec_end,start_point,="
                                 << step_in_star_second << " ,"
                                 << star_time_in_Hour_start_file << " ,"
                                 << star_time_start_end_H << " ,"
                                 << start_time_in_sec_end << " ,"
                                 << start_point << "\n\n";
                     //Запишет в файл системное время
                     fileOut.flush();
                   };//if (otladka==1)

        if (otladka==2)
           {
            OutLine_1="Pnt009 Записали в файл среднемедианные и др. значения";
            ui->LineEdit->setText(OutLine_1);
            //errorMessage.showMessage(OutLine_1);
            //errorMessage.exec();
           };

        //limit_dispersion_min=3.0;
        //Далее переходим к работе с дисперсиями
        //(если разрешено настройками, которые считали из файла name_file_of_ini)

        //Определим, достаточно хороши ли данные, чтобы в них можно было искать дисперсионные импульсы
        if (do_dispers==1)
         {
          //Проверяем качество данных для поиска дисперсий
         };

        if (do_dispers==1)
         {
          //Начинаем глобальный поиск-перебор дисперсий

          std::vector<Point_Impuls_D> Impulses_vec;
             //сюда кладем в итоге найденные импульсы с найденной заметной дисперсией,
             //далее - создаем переменные для его заполнения:
          //Impulses_vec =();

          if (otladka==2)
              {
                 writeStream << "\n Start find dispers. "
                             << "File: " << name_file_of_ini << "\n"
      << "do_dispers, limit_dispersion_min, limit_dispersion_max, limit_myltiply, limit_sigma,  multiply_y, scale_T, add_multiply= "
                 << do_dispers << ", " << limit_dispersion_min << ", "
                 << limit_dispersion_max << ", " << limit_myltiply << ", "
                 << limit_sigma << ", " << multiply_y << ", "
                 << scale_T << ", " << add_multiply << "\n";
                   //Запишет в файл параметры
                 fileOut.flush();
              };//if (otladka==1)

          if (otladka==1)
             {
          OutLine_1="Pnt010 Перед поиском импульсов. countsCanal,startBand,finishBand="
                  + QString::number(countsCanal)
                  + "," + QString::number(startBand)
                  + "," + QString::number(finishBand);
          ui->LineEdit->setText(OutLine_1);
          //errorMessage.showMessage(OutLine_1);
          //errorMessage.exec();
          };


          for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)
          //Проход по лучам БСА - перебираем дисперсии
          {

            //Проверяем качество данных для поиска дисперсий. Если будет плохое, делать ничего не будем.
            counter_bad_data=0;
            counter_bad_dispers_data=0;
            counter_good_bands=0;

            for (unsigned int currentBand = 0; currentBand < n_bands; currentBand++)
              {//Проход по ВСЕМ ХОРОШИМ полосам
                if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)
                       //Из маски =0 - не брать, =1 - брать
                 {
                    counter_good_bands=counter_good_bands+1;
                    for (int cur_median_point = 0; cur_median_point < counter_median_point_in_deriv; cur_median_point++)
                     {
                       if ((media_vec[currentCanal*(n_bands+1)*counter_median_point_in_deriv
                                 +currentBand*counter_median_point_in_deriv+cur_median_point]>limit_flux_max) ||
                           (media_vec[currentCanal*(n_bands+1)*counter_median_point_in_deriv
                                 +currentBand*counter_median_point_in_deriv+cur_median_point]<limit_flux_min))
                            {counter_bad_data=counter_bad_data+1;};
                       if (sigma_data_vec[currentCanal*(n_bands+1)*counter_median_point_in_deriv
                                 +currentBand*counter_median_point_in_deriv+cur_median_point]>limit_dispers)
                                  //Дисперсия (после отброса 30% верхних и 4% нижних значений.
                           //Она примерно в 2 раза меньше, чем реальная)
                           {counter_bad_dispers_data=counter_bad_dispers_data+1;};
                     };//for (int cur_median_point = 0; cur_median_point < counter_median_point_in_deriv; cur_median_point++)
                  };//if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)
              }; //for (unsigned int currentBand = 0; currentBand <= n_bands; currentBand++)

            int tmp_check_1=counter_good_bands*counter_median_point_in_deriv/2;
            //int tmp_check_2=counter_good_bands*counter_median_point_in_deriv/2;
            if ((counter_bad_data>(tmp_check_1)) ||
                 (counter_bad_dispers_data>(tmp_check_1)) ||
                   (counter_good_bands<(n_bands/2)) )
              {
               cod_check_dispers=0;
              }
            else
              {
               cod_check_dispers=1;
              };


            if ((otladka==2)  /*&& (cod_otladka==293)*/)
           {
            log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
            log_out  << "AFTER FIND CHECK parameters: " << name_file_short ;
            log_out  << " cod_check_dispers=" << cod_check_dispers << ", tmp_check_1=" << tmp_check_1;
            log_out  << ", counter_bad_data=" << counter_bad_data << ", counter_bad_dispers_data=" << counter_bad_dispers_data;
            log_out  << " counter_good_bands=" << counter_good_bands
                  << "  counter_median_point_in_deriv=" << counter_median_point_in_deriv;
            log_out  << " \n";
                       /* */
            log_out.close();      // закрываем файл
           };


            if (cod_check_dispers==1)
             {
            prev_stop_time_program= QDateTime::currentDateTime();
            for (unsigned int i_ind=0; i_ind < bsaFile.header.npoints; ++i_ind)
               //Проход по временнЫм точкам - обнуляем значения всех нужных векторов
               {
                  test_vec[i_ind]=0;
                  test_vec_end[i_ind]=0;
                  test_vec_end_tmp[i_ind]=0;
                   //Хранение найденной максимальной величины
                  myltiply_vec[i_ind]=0;
                  dispersion_vec[i_ind]=0;
               };

            counter_good_bands=0;
            limit_down=0;
            for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)
             {//Проход по полосам - ищем начальный суммарный вектор, с которым будем сравнивать
              //- только по хорошим полосам и БЕЗ общей полосы
              if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)
               //Из маски =0 - не брать, =1 - брать
               //Вначале посчитаем из общего многомерного вектора (считанного и откалиброванного куска файла)
               //сумму всех данных в хороших полосах :
                {
                 counter_good_bands=counter_good_bands+1;
                 limit_down=limit_down+data_file_small_sigma[currentCanal*(bsaFile.header.nbands+1)+currentBand]/10.0;
                  //Подсчитываем сумму минимальных порогов величин по хорошим полосам, потом будем подменять ею нули
                 //НО: а надо ли делить на 10? Это - произвол ваще-то... Оставить дисперсию?
                 for (unsigned int i_ind=0; i_ind < bsaFile.header.npoints; ++i_ind)
                   //Проход по временнЫм точкам
                   {
                      test_vec[i_ind]=test_vec[i_ind]+bsaFile.data_file[size_point * i_ind
                                                       + currentCanal * (bsaFile.header.nbands+1)
                                                       + currentBand];
                   };
                };//if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)
             };//for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)
            //Нашли суммарный вектор test_vec() - сумму всех хороших полос на всем большом отрезке
            //Найдем среднее и сигму этого вектора - пойдут в дело при проверках (брать будем только 3*sigma_all_band)

            srednee_all_band=0;
            tmp_double=0;
            for (unsigned int i_ind=0; i_ind < bsaFile.header.npoints; ++i_ind)
                //Проход по временнЫм точкам ВСЕГО считанного временнного отрезка
              {
                tmp_double=tmp_double+test_vec[i_ind];
              };
            srednee_all_band=tmp_double/bsaFile.header.npoints;
            summ=0;
            for (unsigned int i_ind=0; i_ind < bsaFile.header.npoints; ++i_ind)
               //Проход по временнЫм точкам
              {
                tmp_double=test_vec[i_ind]-srednee_all_band;
                summ=summ+ tmp_double*tmp_double;
              };
             //поделим на n-1 значений -- получим среднеквадратичное отклонение
            sigma_all_band=sqrt(summ/(bsaFile.header.npoints-1));
            //control_level_from_sigma_all_band=3.0*sigma_all_band;
            //control_level_from_sigma_all_band=srednee_all_band+3.0*sigma_all_band;
              //ВНИМАНИЕ! ПОТОМ - надо будет варьировать, возможно
            tmp_double=counter_good_bands;
            //limit_sigma_all_band=sqrt(tmp_double);//5.65 - много лишнего, лезут импульсы от коротких помех
            //limit_sigma_all_band=10.0;//- лучше всего отсекает лишнее, хорошие импульсы пульсара с Д=70 видит. Для тестов.
            limit_sigma_all_band=sqrt(tmp_double)*sqrt(2.0);//8.0 для 32 полос, 3,464 - для 6 полос, _базируемся_ на этом

            control_level_from_sigma_all_band=srednee_all_band+limit_sigma_all_band*sigma_all_band;
            //Эту величину мы должны превысить, чтобы зачесть

            //if ((otladka==1) && (currentCanal==4))
            if (otladka==2)
             {
               writeStream << "Produce srednee_all_band, sigma_all_band, limit_sigma_all_band, control_level_from_sigma_all_band = "
                  << srednee_all_band << ", " << sigma_all_band << ", " << limit_sigma_all_band << ", "
                  << control_level_from_sigma_all_band << "\n";
             };

            //На всякий случай проверим все точки вектора, чтобы они не были равны 0 (иначе потом будет деление на ноль)
            //- такие точки приравниваем дисперсии, делённой на 10 (достаточно малое значение, но не равное 0)
            limit_down=limit_down/counter_good_bands;
            //НО: а надо ли было выше делить на 10? Это - произвол ваще-то... Оставить дисперсию?
            for (unsigned int i_ind=0; i_ind < bsaFile.header.npoints; ++i_ind)
              //Проход по временнЫм точкам
              {
                if (test_vec[i_ind]==0)
                   test_vec[i_ind]=limit_down;
              };

            if ((otladka==2) && (currentCanal==4))
             {
                tmp_ind=0;
                tmp_band=31;
                writeStream << "CHECK after SUMM in DISPERS-FIND:  Cnl= "
                   << currentCanal << "\n";
                     //Запишет в файл параметры
                tmp_max=60*80;
                if (tmp_max>bsaFile.header.npoints-1)
                  tmp_max=bsaFile.header.npoints-1;
                while (tmp_ind<=tmp_max)
                  {
                    writeStream << tmp_ind << ": "
                         << test_vec[tmp_ind] << ", " ;
                    tmp_ind=tmp_ind+80;
                  };
                writeStream << "\n";
              };//if (otladka==1)

            //Найдем крайние рабочие полосы снизу и сверху для данного луча диаграммы
            int startBand_tmp = -1; //перед поиском первой рабочей частоты ставим заведомо неверный индекс
            startBand=0;
            unsigned int current_Band=0;
            while (startBand_tmp < 0)
             {
              if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+current_Band]==1)
            //Из маски =0 - не брать, =1 - брать
                {
                 startBand_tmp=current_Band;
                };
              current_Band=current_Band+1;
             };
            startBand=startBand_tmp;
            //Захватили начальную полосу (номер), в которой есть хорошие данные

            int finishBand_tmp=-1;  //перед поиском последней рабочей частоты ставим заведомо неверный индекс
            current_Band=bsaFile.header.nbands-1;
            while (finishBand_tmp < 0)
             {
              if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+current_Band]==1)
            //Из маски =0 - не брать, =1 - брать
                {
                 finishBand_tmp=current_Band;
                };
              current_Band=current_Band-1;
             };
            finishBand=finishBand_tmp;
            //Захватили финальную полосу (номер), в которой есть хорошие данные

            //finishBand = bsaFile.header.nbands;

            //Теперь ищем превышения при условии, что мы перебираем дисперсии, и смотрим, чтобы превышения
            //превышали некий уровень (обычно в 3 раза больше начального), и к тому же сама величина была
            //больше 5 сигм (обычно, эта величина задается) на 20-секундном отрезке
            if (otladka==2)
              {
                writeStream << "PERED WORK: npoints , check_npoints  = "
                            << bsaFile.header.npoints << ", " << check_npoints
                            << " \n"; //Запишет в файл параметры
                fileOut.flush();
              };//if (otladka==1)

            //Главный перебор - по сетке дисперсий. Это - ПЕРВЫЙ перебор по всей сетке дисперсий.
            //Потом будет еще уточняющий перебор, в функции
            //limit_dispersion_min=8.0;
            for (unsigned int ind_dispers= 0; ind_dispers < 100; ind_dispers++)
             {
              if ((dispersion_maska[ind_dispers]>=limit_dispersion_min)
                        && (dispersion_maska[ind_dispers]<=limit_dispersion_max))
               {//Ищет только в заданных (извне) пределах. Поскольку, как показывает практика,
                //меньше 8-10 дисперсию искать бесполезно - много грязи

                for (int i_ind=0; i_ind < check_npoints; ++i_ind)
                   //Проход по временнЫм точкам - присваиваем начальные значения конечного вектора
                    //в интервале нужных индексов
                   // Примечание: из самой высокой частоты (вектор (мог) принял(-ть)
                    //ненулевое значение на предыдущем цикле)
                 {
                      test_vec_end[i_ind]=bsaFile.data_file[size_point * i_ind
                              + currentCanal * (bsaFile.header.nbands+1)
                              + finishBand];
                 };

                /*
                //ТЕСТЫ
                for (unsigned int currentBand = startBand ; currentBand <=  finishBand-1; currentBand++)
                 {//Проход по полосам - ищем начальный суммарный вектор, с которым будем сравнивать
                  //- только по хорошим полосам и БЕЗ общей полосы
                  //Хотел спускаться по полосам сверху, но не получилось...
                   if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)
                     //Из маски =0 - не брать, =1 - брать
                    {
                      d_T_in_band=4.15*1000*dispersion_maska[ind_dispers] *
                         ( pow(bsaFile.header.fbands[currentBand],-2) - pow(bsaFile.header.fbands[finishBand],-2) );
                      //Дисперсионная_Задержка_в_сек=4,15*1000*Dispers*(СТЕПЕНЬ(frec_min;-2) - СТЕПЕНЬ(frec_max;-2))
                      //Где frec_min и frec_max - в МГц
                      // Дисперсионная_Задержка_в_сек=4,15*1000*Dispers* ((pow(frec_min,-2) - (pow(frec_max,-2))
                      d_X=d_T_in_band*(1000.0/bsaFile.header.tresolution);

                      d_ind_x1=int(d_X);
                      d_ind_x2=d_ind_x1+1;
                      d_X_after_left_ind=d_X-d_ind_x1;

                      if ((otladka==3) && (currentCanal==0))
                        {
                          writeStream << "TEST ray,band, Frec_1, Frec_2, ind_D, dispersion,d_T, d_X = "
                                      << currentCanal << ", " << currentBand << ", "
                                      << bsaFile.header.fbands[currentBand] << ", "
                                      << bsaFile.header.fbands[finishBand] << ", "
                                      << ind_dispers << ", "
                                      << dispersion_maska[ind_dispers] << ", "
                                      << d_T_in_band << ", " << d_X << ", "
                                      << " \n"; //Запишет в файл параметры
                          fileOut.flush();
                        };//if (otladka==1)

                    }//if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)

                  };//for (unsigned int currentBand = startBand ; currentBand <=  finishBand-1; currentBand++)
                //КОНЕЦ ТЕСТЫ
                */


                //Теперь найдем суммарный вектор в нужном диапазоне для хороших полос - ДЛЯ ДАННОЙ ДИСПЕРСИИ
                for (unsigned int currentBand = startBand ; currentBand <=  finishBand-1; currentBand++)
                 {//Проход по полосам - ищем начальный суммарный вектор, с которым будем сравнивать
                  //- только по хорошим полосам и БЕЗ общей полосы
                  //При этом мы _спускаемся_ по полосам, начиная от самой высокочастотной.
                  //Хотел спускаться по полосам, но не получилось...
                   if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)
                     //Из маски =0 - не брать, =1 - брать
                    {
                      d_T_in_band=4.15*1000*dispersion_maska[ind_dispers] *
                         ( pow(bsaFile.header.fbands[currentBand],-2) - pow(bsaFile.header.fbands[finishBand],-2) );
                      //Дисперсионная_Задержка_в_сек=4,15*1000*Dispers*(СТЕПЕНЬ(frec_min;-2) - СТЕПЕНЬ(frec_max;-2))
                      //Где frec_min и frec_max - в МГц
                      // Дисперсионная_Задержка_в_сек=4,15*1000*Dispers* ((pow(frec_min,-2) - (pow(frec_max,-2))
                      d_X=d_T_in_band*(1000.0/bsaFile.header.tresolution);

                      d_ind_x1=int(d_X);
                      d_ind_x2=d_ind_x1+1;
                      d_X_after_left_ind=d_X-d_ind_x1;

                      //Внимание! Начиная с D>26 для 32 полос и >39 для 6 полос
                      //- разница во времени между двумя соседними полосами становится больше разрешения 12.5 или 100 мс
                      //Это граничное значение хранится в limit_round_Disper
                      ind_2=int(d_X+0.5);
                      int do_interpolation=1;


                      if (otladka==2)
                        {
                          writeStream << "ray,band, ind_D, dispersion,d_T, d_X = "
                                      << currentCanal << ", " << currentBand << ", "
                                      << ind_dispers << ", "
                                      << dispersion_maska[ind_dispers] << ", "
                                      << d_T_in_band << ", " << d_X << ", "
                                      << " \n"; //Запишет в файл параметры
                          fileOut.flush();
                        };//if (otladka==1)


                      if (do_interpolation==1)
                       {
                      //Распараллелим через цикл по i_ind ... check_npoints
                      //#pragma omp parallel for
                          //Не дало нужного приращения! Наоборот, замедлило в 2 раза...
                        for (int i_ind=0; i_ind < check_npoints; ++i_ind)
                         //Проход по временнЫм точкам - присваиваем начальные значения конечного вектора в
                          //интервале нужных индексов
                        {
                         /* */ //ПОСЛЕДОВАТЕЛЬНАЯ СЕКЦИЯ, через интерполяцию точек по бокам дробного значения индекса:
                         y1 = bsaFile.data_file[size_point * (i_ind+d_ind_x1)
                                 + currentCanal * (bsaFile.header.nbands+1)
                                 + currentBand];
                         y2 = bsaFile.data_file[size_point * (i_ind+d_ind_x2)
                                 + currentCanal * (bsaFile.header.nbands+1)
                                 + currentBand];
                         //calib_data_T=y1 + (y2 - y1) * (i_ind - x1) / (x2 - x1) ;
                         calib_data_T=y1 + (y2 - y1) * d_X_after_left_ind;
                         test_vec_end[i_ind]=test_vec_end[i_ind]+calib_data_T;
                        /* */
                         //Прибавили интерполированное значение


                          /* //Параллельная секция-1
                         test_vec_end[i_ind]=test_vec_end[i_ind]+
                                 bsaFile.data_file[size_point * (i_ind+d_ind_x1)
                                 + currentCanal * (bsaFile.header.nbands+1)
                                 + currentBand]+
                                 (bsaFile.data_file[size_point * (i_ind+d_ind_x2)
                                 + currentCanal * (bsaFile.header.nbands+1)
                                 + currentBand]
                                 - bsaFile.data_file[size_point * (i_ind+d_ind_x1)
                                 + currentCanal * (bsaFile.header.nbands+1)
                                 + currentBand]) * d_X_after_left_ind;
                                 */
                            //Прибавили интерполированное значение
                        } ;//for (unsigned int i_ind=0; i_ind < check_npoints; ++i_ind)
                       }//if (do_interpolation==1)
                      else
                       {
                          //Распараллелим через цикл по i_ind ... check_npoints
                         //#pragma omp parallel for
                          //Не дало нужного приращения! Наоборот, замедлило в 2 раза...
                         for (int i_ind=0; i_ind < check_npoints; ++i_ind)
                           //Проход по временнЫм точкам - присваиваем начальные значения конечного вектора в
                            //интервале нужных индексов
                          {
                           /* */ //ГРУБО: просто ближайшая по сдвигу индекса точка:
                            test_vec_end[i_ind]=test_vec_end[i_ind]+bsaFile.data_file[size_point * (i_ind+ind_2)
                                   + currentCanal * (bsaFile.header.nbands+1)
                                   + currentBand];

                            /* //Параллельная секция = последовательной   */

                          } ;//for (unsigned int i_ind=0; i_ind < check_npoints; ++i_ind)

                       };//else if (do_interpolation==1)

                    };//if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)
                     //Прибавляем все хорошие полосы

                 };//for (unsigned int currentBand = finishBand-1 ; currentBand >= startBand; currentBand--)

                //Теперь проанализируем то, что накопилось в контрольном векторе после суммирования всех полос
                //и сравним с накопленной ранее матрицей данных - кандидатов во всплески с дисперсией


                for (int cur_median_point = 0;
                     cur_median_point<counter_median_point_in_deriv-1; cur_median_point++)
                  {
                    sigma=0;
                    counter_good_bands=0;
                    for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)
                     {//Проход по полосам - ищем среднюю сигму по всем хорошим полосам
                       if (data_file_filter[currentCanal*(bsaFile.header.nbands+1)+currentBand]==1)
                                    //Из маски =0 - не брать, =1 - брать
                        {
                          sigma=sigma+sigma_data_vec[currentCanal*(bsaFile.header.nbands+1)*counter_median_point_in_deriv
                                          +currentBand*counter_median_point_in_deriv+cur_median_point];
                          counter_good_bands=counter_good_bands+1;
                        };
                      };//for (unsigned int currentBand = startBand; currentBand <= finishBand; currentBand++)

                    control_level=(sigma/counter_good_bands)*limit_sigma;
                    //Усредненную сигму (станлартное уклонение от среднего) по всем хорошим полосам

                    if (cur_median_point==0)
                     {
                      ind_beg=0;
                      ind_end=ind_in_step_for_median_data;
                     }
                    else
                     {
                      ind_beg=ind_end+1;
                      ind_end=ind_end+ind_in_step_for_median_data;
                     };//else if (cur_median_point==0)

                    for (int i_ind=ind_beg; i_ind <= ind_end; ++i_ind)
                   //Проход по временнЫм точкам - проверяем значения конечного вектора в интервале нужных индексов
                   // - превосходит ли он лимиты и предыдущее значение?
                     {
                       if ((test_vec_end[i_ind]>control_level) && (test_vec_end[i_ind]>control_level_from_sigma_all_band))
                       {//Разделяем условия, чтобы проверять не все и отсекать вначале то, что легче и чаще проверить
                        //Главное - чтобы был выступ более 5 сигм, определенное как среднее по полосам (причем с просеяяными на выбросы),
                        //и более 3-х сигм сигмы (определенной "честно". без отбрасывания крайних значений) вектора СУММЫ всех полос

                          //OLD epsilon=test_vec_end[i_ind]/test_vec[i_ind];
                          //??? epsilon=(test_vec_end[i_ind]-srednee_all_band)/(test_vec[i_ind]-srednee_all_band);
                          epsilon=(test_vec_end[i_ind]-srednee_all_band)/sigma_all_band;

                          //if ( (epsilon>myltiply_vec[i_ind]) && (epsilon>limit_myltiply) )
                          if ( (test_vec_end_tmp[i_ind]<test_vec_end[i_ind]) && (epsilon>limit_myltiply) )
                           {

                              if ((otladka==2) and (currentCanal==4))
                                {
                  writeStream << "FIND GOOD! ray, i, Ds_DO, Ds_PO, Pre_V[i], DO_V[i], PO_V[i], DO_*, PO_* = "
                                     << currentCanal << ", " << i_ind << ", "
                                     << dispersion_vec[i_ind] << ", " << dispersion_maska[ind_dispers] << ", "
                                     << test_vec[i_ind] << ", " << test_vec_end_tmp[i_ind] << ", " << test_vec_end[i_ind] << ", "
                                     << myltiply_vec[i_ind] << ", " << epsilon
                                     << " \n"; //Запишет в файл параметры
                                  fileOut.flush();
                                };//if (otladka==1)

                              myltiply_vec[i_ind]=epsilon;
                              dispersion_vec[i_ind]=dispersion_maska[ind_dispers];
                              test_vec_end_tmp[i_ind]=test_vec_end[i_ind];
                               //Хранение найденной максимальной величины
                           };//if ( (test_vec_end_tmp[i_ind]<test_vec_end[i_ind]) && (epsilon>limit_myltiply) )
                       };//if ((test_vec_end[i_ind]>control_level) && (test_vec_end[i_ind]>control_level_from_sigma_all_band))
                     };//for (unsigned int i_ind=ind_beg; i_ind <= ind_end; ++i_ind)
                  };//for (unsigned int cur_median_point = 0;
                      //cur_median_point<counter_median_point_in_deriv-1; cur_median_point++)

                if (otladka==2)
                  {
                    writeStream << "CHECKING ray, Dispers, First_vec[0], End_vec[0], First_vec[end], End_vec[end] = "
                                << currentCanal << ", " << dispersion_maska[ind_dispers] << ", "
                                << test_vec[0] << ", " << test_vec_end[0] << ", "
                                << test_vec[check_npoints-1] << ", " << test_vec_end[check_npoints-1]
                                << " \n"; //Запишет в файл параметры
                    fileOut.flush();
                  };//if (otladka==1)

                };//if ((dispersion_maska[ind_dispers]>=limit_dispersion_min)
                 //&& (dispersion_maska[ind_dispers]<=limit_dispersion_max))
              };//for (unsigned int ind_dispers= 0; ind_dispers < 100; ind_dispers++)
              //Главный перебор - по сетке дисперсий - ЗАКОНЧЕН

            if (otladka==2)
              {
                 stop_time_program = QDateTime::currentDateTime();
                 writeStream << "\n WORK WITH DISPERS, at ray= "
                             << currentCanal << ", time="
                             << QString::number(stop_time_program.msecsTo(start_time_program))
                             << " msec from start and " //Запишет в файл параметры
                             << QString::number(stop_time_program.msecsTo(prev_stop_time_program))
                             << " msec from previous proc.\n"; //Запишет в файл параметры
                 fileOut.flush();
              };//if (otladka==1)

            counter_point_in_Dispers_array=0;
            for (int i_ind=0; i_ind < check_npoints; ++i_ind)
               //Проход по накопленным точкам - выводим их в файл-лог
               {
                  if (myltiply_vec[i_ind]>limit_myltiply)
                    {
                      //if ((otladka==1) and (currentCanal==4))
                      if (otladka==2)
                        {
                          tmp_double=(test_vec_end_tmp[i_ind]-srednee_all_band)/sigma_all_band;
                          writeStream << "RESULT ray,i,mylt_coef, D_1, D_e_max, D_e_last, Base_mylt* , dispers= "
                                      << currentCanal << ", " << i_ind << ", "
                                      << myltiply_vec[i_ind] << ",  "
                                      << test_vec[i_ind] << ", " << test_vec_end_tmp[i_ind] << ", " << test_vec_end[i_ind] << ",   "
                                      << tmp_double << ", " << dispersion_vec[i_ind]
                                      << " \n"; //Запишет в файл параметры
                          fileOut.flush();
                        };//if (otladka==1)
                      counter_point_in_Dispers_array=counter_point_in_Dispers_array+1;
                    };//if (myltiply_vec[i_ind]>limit_myltiply)
               };//for (unsigned int i_ind=0; i_ind < check_npoints; ++i_ind)

            if (otladka==2)
              {
                writeStream << "RESULT of FIND DISPERSION in ray : "
                            << currentCanal << ", count=" << counter_point_in_Dispers_array
                            << " \n"; //Запишет в файл параметры
                fileOut.flush();
              };//if (otladka==1)
            counter_point_in_Dispers_array_total=counter_point_in_Dispers_array_total+counter_point_in_Dispers_array;

            if (counter_point_in_Dispers_array>0)
              counter_point_in_Dispers_array_total_in_ALL_RAYS=counter_point_in_Dispers_array_total_in_ALL_RAYS+1;

            //НАЧАЛО Теперь проанализируем найденный суммарный вектор, просмотрим, при какой дисперсии
            //наилучшее усиление
            //, поищем более точное значение там и отрисуем кое-что наилучшее из найденного

            if (otladka==2)
               {
            OutLine_1="Pnt010 Перед уточнением найденного импульса. start_time_in_sec_end,multiply_y,scale_T,add_multiply,srednee_all_band,sigma_all_band,limit_sigma_all_band,limit_sigma="
                    + QString::number(start_time_in_sec_end,'f', 4)
                    + "," + QString::number(multiply_y,'f', 4)
                    + "," + QString::number(scale_T,'f', 4)
                    + "," + QString::number(add_multiply,'f', 4)
                    + "," + QString::number(srednee_all_band,'f', 4)
                    + "," + QString::number(sigma_all_band,'f', 4)
                    + "," + QString::number(limit_sigma_all_band,'f', 4)
                    + "," + QString::number(limit_sigma,'f', 4)
            + "," + name_file.c_str()+ "," + name_file_short.c_str()+ "," + QString::number(device)
            + "," + QString::number(n_bands) + "," + QString::number(resolution) + "," + QString::number(start_point)
            + "," + QString::number(mjd_start,'f', 4)+ "," + QString::number(star_time_in_Hour_start_file,'f', 4)
            + "," + now.toString("yyyy.MM.dd hh:mm:ss.zzz")
            + "," + QString::number(size_point) + "," + QString::number(currentCanal)
            + "," + QString::number(startBand) + "," + QString::number(finishBand)
            + "," + QString::number(counter_good_bands);

            ui->LineEdit->setText(OutLine_1);
            errorMessage.showMessage(OutLine_1);
            errorMessage.exec();
            };

            //std::ofstream log_out;                    //создаем поток
            log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
            log_out  << bsaFile.name_file_of_datas.toStdString()  << " : posle 1-go poiska disperssii. \n" ;      // сама запись

            stop_time_program = QDateTime::currentDateTime();
            counter_ms_from_start=stop_time_program.msecsTo(stop_time_program)-stop_time_program.msecsTo(start_time_program);
            log_out  << "Work_TIME_in_millisec=" << counter_ms_from_start << " millisec.\n" ;

            log_out.close();      // закрываем файл

            counter_otladka=counter_otladka+1;
            if ((otladka==1)  && (counter_otladka>=292))
               {
            log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
            log_out  << bsaFile.name_file_of_datas.toStdString()  << " : PERED 2-go poiskom disperssii: \n" ;
            // сама запись
            log_out  << bsaFile.name_file_of_datas.toStdString()
                     << " : PARAMETERS:  ===\n";
            log_out  << " counter_otladka=" << counter_otladka;
            log_out  << " add_tmp=" << add_tmp;
            log_out  << ", start_time_in_sec=" << start_time_in_sec_end
                     << " ; multiply_y=" << multiply_y  << "\n";      // сама запись
            log_out  << ", scale_T=" << scale_T;
            log_out  << ", add_multiply=" << add_multiply;
            log_out  << ", srednee_all_band=" << srednee_all_band;
            log_out  << ", sigma_all_band=" << sigma_all_band;
            log_out  << ", limit_sigma_all_band=" << limit_sigma_all_band;
            log_out  << ", limit_sigmaT=" << limit_sigma;
            log_out  << ", name_file=" << name_file  << ",";
            log_out  << ", name_file_short=" << name_file_short << ",";
            log_out  << ", device=" << device;
            log_out  << ", n_bands=" << n_bands;
            log_out  << ", resolution=" << resolution;
            log_out  << ", start_point=" << start_point;
            log_out  << ", mjd_start=" << mjd_start;
            log_out  << ", star_time_in_Hour_start_file=" << star_time_in_Hour_start_file;

            QString str_tmp=now.toString("yyyy.MM.dd hh:mm:ss.zzz");
            log_out  << ", now=" << str_tmp.toStdString();
            log_out  << ", size_point=" << size_point;
            log_out  << ", start_point=" << start_point;
            log_out  << ", currentCanal=" << currentCanal;
            log_out  << ", startBand=" << startBand;
            log_out  << ", finishBand=" << finishBand;
            log_out  << ", counter_good_bands=" << counter_good_bands;
            log_out  << " \n\n";
                           /* */

            log_out.close();      // закрываем файл
            };

            FindImpulses_in_Data(0,add_tmp,400,/* start_time_in_sec - НЕТ!!! */ start_time_in_sec_end,
                                 multiply_y,scale_T,add_multiply,
                                 srednee_all_band,sigma_all_band,
                                 limit_sigma_all_band,limit_sigma,
                                 name_file,name_file_short,device,
                                 n_bands,resolution,/*ray,*/start_point,
                                 mjd_start,star_time_in_Hour_start_file,now,
                                 size_point,currentCanal,
                                 startBand, finishBand,
                                 counter_good_bands,data_file_filter,
                                 //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос
                                 dispersion_maska,myltiply_vec,dispersion_vec,
                                 test_vec, test_vec_end,test_vec_end_tmp,Impulses_vec,counter_otladka);
             /*Добавили в вектор импульсов очередной найденный импульс, и отрисовали его, если он хорош */

            //std::ofstream log_out;                    //создаем поток
            log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
            log_out  << bsaFile.name_file_of_datas.toStdString()  << " : posle 2-go poiska disperssii. \n" ;      // сама запись

            stop_time_program = QDateTime::currentDateTime();
            counter_ms_from_start=stop_time_program.msecsTo(stop_time_program)-stop_time_program.msecsTo(start_time_program);
            log_out  << "Work_TIME_in_millisec=" << counter_ms_from_start << " millisec.\n" ;

            log_out.close();      // закрываем файл
            if (otladka==2)
               {

            OutLine_1="Pnt010a now AFTER find Impuls N == "
                    + now.toString("yyyy.MM.dd hh:mm:ss.zzz")
                    + QString::number(Impulses_vec.size());
            ui->LineEdit->setText(OutLine_1);
            errorMessage.showMessage(OutLine_1);
            errorMessage.exec();


            if (Impulses_vec.size()>0)
            {
            //Current_Impuls=Impulses_vec.at(i);

            OutLine_1= "Pnt010b Impuls N == "
                         + QString::number(Impulses_vec.size()) + Impulses_vec.at(Impulses_vec.size()-1).name_file_short.c_str() ;
            OutLine_1 = OutLine_1 + ";" + Impulses_vec.at(Impulses_vec.size()-1).name_file_graph.c_str() + ";" +
                    QString::number(Impulses_vec.at(Impulses_vec.size()-1).device,'f', 0) + ";" +
                    QString::number(Impulses_vec.at(Impulses_vec.size()-1).n_bands,'f', 0) + ";" +
                    QString::number(Impulses_vec.at(Impulses_vec.size()-1).ray,'f', 0) + ";" +
                    QString::number(Impulses_vec.at(Impulses_vec.size()-1).ind,'f', 0) + ";" +
                    QString::number(Impulses_vec.at(Impulses_vec.size()-1).MJD,'f', 8) + ";" +
                    Impulses_vec.at(Impulses_vec.size()-1).UT.toString("yyyy:MM:dd hh:mm:ss.zzz") +
                    ";" + QString::number(Impulses_vec.at(Impulses_vec.size()-1).Star_Time,'f', 6) +
                    ";" + QString::number(Impulses_vec.at(Impulses_vec.size()-1).d_T_zona,'f', 3) +
                    //d_T_zona=(ind_righte-ind_left)*resolution/1000.0;
                    //нашли зону распространения импульса (грубо) в секундах
                    ";" + QString::number(Impulses_vec.at(Impulses_vec.size()-1).d_T_Half_H,'f', 3) +
                    ";" + QString::number(Impulses_vec.at(Impulses_vec.size()-1).max_Data,'f', 3) +
                    ";" + QString::number(Impulses_vec.at(Impulses_vec.size()-1).S_to_N,'f', 2) +
                    ";" + QString::number(Impulses_vec.at(Impulses_vec.size()-1).Dispers,'f', 2) +
                    ";" + QString::number(Impulses_vec.at(Impulses_vec.size()-1).n_similar_Impuls_in_rays,'f', 0) +
                    ";" + QString::number(Impulses_vec.at(Impulses_vec.size()-1).blended,'f', 0) + "\n";

            ui->LineEdit->setText(OutLine_1);
            //errorMessage.showMessage(OutLine_1);
            //errorMessage.exec();
            };// if (Impulses_vec.size()>0)

            };
             //if (otladka==2)
            //КОНЕЦ

            };
            //if (cod_check_dispers==1)

        };//for (unsigned int currentCanal = 0; currentCanal < countsCanal; currentCanal++)

            //СДЕЛАЛИ Проход по лучам БСА - перебирали дисперсии и добавляли новые импульсы
          //в вектор импульсов Impulses_vec
          if (otladka==1)
            {
               writeStream << "PERED IMPULSAMI i, nn_imp="
                           << i << ", " << Impulses_vec.size()
                           << "\n"; //Запишет в файл параметры

            };//if (otladka==1)

          //if (Impulses_vec.size()>0) //если список импульсов непустой, работаем с ним
            {
              //Далее просмотрим список накопленных импульсов и маркируем похожие (в соседних лучах и бленды на одном луче):
              Analyze_and_rewrite_ArrayImpulses(countsCanal,Impulses_vec);

              //std::ofstream log_out;                    //создаем поток
              log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
              log_out  << bsaFile.name_file_of_datas.toStdString()  << " : posle analiza spiska disperssii. \n" ;      // сама запись
              log_out.close();      // закрываем файл


              if (otladka==2)
                 {
              OutLine_1="Pnt011 Перед записью в файл найденных импульсов. mjd_start,out_data_dir,counter_median_point_in_deriv,deriv_star="
                      + QString::number(mjd_start,'f',4) +  "," + out_data_dir.c_str()
                      + "," + QString::number(counter_median_point_in_deriv)
                      + "," + QString::number(deriv_star,'f', 4);
              ui->LineEdit->setText(OutLine_1);
              errorMessage.showMessage(OutLine_1);
              errorMessage.exec();
              };

              //Далее надо записать список накопленных импульсов в текстовый файл:
              Write_ArrayImpulses_in_file(out_data_dir,device,countsCanal,n_bands,
                                                resolution,
                                                bsaFile.header.date_begin, // Дата начала регистрации файла в UTC, считано ранее из заголовка
                                                bsaFile.header.time_begin, // Время начала регистрации файла в UTC, считано ранее из заголовка
                                                step_in_star_second,//шаг в звездных секундах
                                                star_time_in_Hour_start_file,//стартовая ЗВЕЗДНАЯ секунда НАЧАЛА часа (файла)
                                                star_time_start_end_H,//стартовая ЗВЕЗДНАЯ секунда часа, с которой все начинается
                                                start_time_in_sec_end,//стартовая ВРЕМЕННАЯ секунда часа, с которой все начинается
                                                start_point,
                                                mjd_start,
                                                counter_median_point_in_deriv,//точек в массивах
                                                i, //Номер считанного КУСКА-порции файла данных по counter_median_point_in_deriv
                                                deriv_star,
                                           // количество сдвиговых секунд,вычисленных из ЗВЕЗДНЫХ секунд в порции данных
                                           //- реально их должно быть меньше, чтобы не было разрывов в данных
                                                Impulses_vec);
               /*
                Write_ArrayImpulses_in_file(std::string out_data_dir,
                                                 int device, int countsCanal, int n_bands,
                                                 float resolution,
                                                 const QString date_begin, // Дата начала регистрации файла в UTC, считано ранее из заголовка
                                                 const QString time_begin, // Время начала регистрации файла в UTC, считано ранее из заголовка
                                                 const float step_in_star_second,//шаг в звездных секундах
                                                 const double star_time_in_Hour_start_file,//стартовая ЗВЕЗДНАЯ секунда НАЧАЛА часа (файла)
                                                 const double star_time_start_end_H,//стартовая ЗВЕЗДНАЯ секунда часа, с которой все начинается
                                                 const double start_time_in_sec_end,//стартовая ВРЕМЕННАЯ секунда часа, с которой все начинается
                                                 const long start_point,
                                                 const double mjd_start,
                                                 const int counter_median_point_in_deriv,//точек в массивах
                                                 std::vector<Point_Impuls_D> &Impulses_vec)  */
           };//if (Impulses_vec.size()>0)




         };
        //if (do_dispers==1)
        //Закончили работу с дисперсиями

        if (otladka==1)
           {
            OutLine_1="Pnt012 Нашли и записали в файл дисп. импульсы для ";
            OutLine_1=OutLine_1 + bsaFile.name_file_of_datas;
            ui->LineEdit->setText(OutLine_1);
            //errorMessage.showMessage(OutLine_1);
            //errorMessage.exec();
           };
        if (otladka==2)
          {
             stop_time_program = QDateTime::currentDateTime();
             writeStream << "\n END OF ALL WORK= "
                         << ", time="
                         << QString::number(stop_time_program.secsTo(start_time_program))
                         << " sec from start and " //Запишет в файл параметры
                         << QString::number(stop_time_program.secsTo(prev_stop_time_program))
                         << " sec from previous proc. Counter_point_in_Dispers_array, Count_GOOD_RAYS = "
                         << counter_point_in_Dispers_array_total << ", "
                         << counter_point_in_Dispers_array_total_in_ALL_RAYS << "\n"; //Запишет в файл параметры
             fileOut.flush();
          };//if (otladka==1)

        /*
        progress_ptr->setValue(100.0 * (i+1) * deriv / 3600);
        */


        if (read_from_file_or_list_names_of_files==0)
         //Чтение из предложенного файла: увеличиваем счетчик считываемой порции внутри данного файла
         {
            i=i+1;
         };




        stop_time_program = QDateTime::currentDateTime();
        float counter_sec_from_start=stop_time_program.secsTo(stop_time_program)-stop_time_program.secsTo(start_time_program);
        log_out.open("D:\\DATA\\LOGS\\log_01.txt", std::ios::app);  // открываем файл для записи в конец
        log_out  << bsaFile.name_file_of_datas.toStdString()  << " : end of cicle listing. " ;
        log_out  << "; WORK_TIME_in_sec=" << counter_sec_from_start << " seconds. \n" ;
        // сама запись
        log_out.close();      // закрываем файл

        };//if (alarm_cod_of_read==0)
        //работали над очередной порцией данных во времени (обычно порции по 1-4 минуты)




        qApp->processEvents();// обработка ожидающих в очереди событий
      }; //for (unsigned int i=0; i < max_i && !stopThread; i++)
      //i здесь - номер порции для обработки (обычно порции по 4 минуты, т.е. до i<15)
    //while ( (i< max_i) && (alarm_cod_of_read==0) && (!stopThread)) //Заменили цикл на предусловие, чтобы смочь отслеживать лист файлов

//+++++++++++++++КОНЕЦ обработки - прохода по заданным временным отрезкам+++++++++++++++++++++++++++++++++++++++++

/*
    progress_ptr->hide();
    //Закрыли меню на экране
    */

    if (otladka==1)
       {
    OutLine_1="Pnt013 Выходим из процедуры обработки";
    ui->LineEdit->setText(OutLine_1);
    errorMessage.showMessage(OutLine_1);
    errorMessage.exec();
    };



}




/*
*************************************************************************************************************
 * Главная функция обработки - ниже.
*************************************************************************************************************
 */
void grav::on_pushButton_4_clicked()
{//Сюда добавляем функции при нажатии на кнопку обработки "Proc"
    long n_single_points;


    std::string name_file,name_file_short;
    int device,n_bands,ray,ind;
    float resolution;
    double MJD;
    QDateTime UT;
    float Star_Time, d_T_zona, d_T_Half_H, max_Data, S_to_N, Dispers;
    int n_similar_Impuls_in_rays;
     //создали переменные для заполнения единичного Point_Impuls_D

    //????????????????
    double start_time_in_sec = ui->doubleSpinBox->value();
       // стартовая секунда, с которой надо отрисовывать файлы порций;
    //??????????????????
    if(bsaFile.name_file_of_datas.isEmpty())
        return;

    int do_dispers=1;//=1 - РАБОТАТЬ с дисперсиями
    float add_tmp=0;
    float multiply_y=1.0;
    float scale_T=1000.0;
    float add_multiply=0.75;
            /*
            * multiply_y  - масштабный коэффициент графика по Y (чем больше, тем крупнее масштаб)
            *          было ранее multiply_y = ui->doubleSpinBox_2->value()
            * scale_T - масштабный отрезок температуры, который пририсовывается справа внизу по правой оси Y
            * (и на данных впритык к ней)
            *    ,было ранее: scale_T = ui->doubleSpinBox_3->value();
            * add_multiply - сжимающий графики по оси Y коэффициент - втискивает графики из многих строк в окно
            *                   было ранее: add_multiply=ui->doubleSpinBox_4->value();
            *
            */

    std::string out_data_dir, dispers_data_file;
    float limit_dispersion_min=10.0;
    float limit_dispersion_max=2000.0;
    float limit_myltiply=3.0;
    float limit_sigma=5.0;
      //Принимаем стандартные 5 сигм - должен произойти выброс в конкретной полосе частот более этого значения,
      // чтобы мы его "зацепили" (то есть учли бы)
    float limit_Alpha_min,limit_Alpha_max,limit_Delta_min,limit_Delta_max;
    std::string limit_UT_min,limit_UT_max,list_names_of_files,names_of_file_EQ;
    int read_from_file_or_list_names_of_files;

    //Отрисовываем файл по кускам длиной spinBox->value() секунд в графические файлы
    unsigned int deriv = ui->spinBox->value(); // количество секунд в порции

    name_file_of_ini=ui->LineEdit_fileIni->text();
    read_from_file_ini_for_find_of_Dispers(name_file_of_ini,
                                           do_dispers,
                                           limit_dispersion_min,limit_dispersion_max,
                                           limit_myltiply,limit_sigma,
                                           multiply_y,scale_T,add_multiply,deriv,
                                           out_data_dir,dispers_data_file,
                                           limit_Alpha_min,limit_Alpha_max,limit_Delta_min,
                                           limit_Delta_max,limit_UT_min,limit_UT_max,
                                           read_from_file_or_list_names_of_files,
                                           list_names_of_files,names_of_file_EQ);
    //Из файла name_file_of_ini считали настройки для работы с дисперсиями





    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    bsaFile.header.npoints = npoints;
                   //число точек (временнЫх, т.е. векторов!) в файле данных, в идеале = 36000 или 288000
    int nCountOfModul = (bsaFile.header.nbands+1) * bsaFile.nCanal;
                   // количество (-лучей-) точек данных в одном модуле из 8 лучей - обычно 56 или 264
    long nCountPoint = nCountOfModul * bsaFile.header.modulus.size();
        // количество (-лучей-) точек данных во всех модулях - ТО ЕСТЬ - сколько единичных точек в одной временнОй точке
        //- обычно или 48(лучей)*7(полос)= 336 или 48*33= 1584
    long nCounts = bsaFile.header.npoints*nCountPoint;
              // количество _единичных_ точек данных во всём файле, в идеале 36000*336 или 288000*1584
    unsigned int countsCanal = bsaFile.header.modulus.size() * bsaFile.nCanal;
                           // количество (-каналов-) лучей диаграммы БСА в файле - практически всегда = 48

    int size_point = (bsaFile.header.nbands+1) * bsaFile.nCanal * bsaFile.header.modulus.size();
        //Число данных файла в 1 временной точке, для 6 полос и 48 лучей =336
        //Надо будет сдвинуться на НУЖНОЕ начало двоичных данных, с которого начнем считывание, примерно так:
        // пример:  inClientFile.seekg(bsaFile.sizeHeader + (start_point*size_point) * sizeof(float), std::ios::beg);

    // открываем файл с данными:
    std::ifstream inClientFile(bsaFile.name_file_of_datas.toStdString().c_str(), std::ios::binary);
 //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // устанавливаем позицию на начало двоичных данных
    //inClientFile.seekg(bsaFile.sizeHeader, std::ios::beg); - ПОТОМ!

    // загрузка данных
    QDateTime start_time_program = QDateTime::currentDateTime();
      //Захватили время старта подпрограммы
    QDateTime stop_time_program, prev_stop_time_program;

    /*
    //!!!!!!!!!!!
    QProgressDialog* progress_ptr = new QProgressDialog("Формируем файлы ...", "Отмена", 0, 100, this);
    connect(progress_ptr, SIGNAL(canceled()), this, SLOT(on_progress_cancel()));
    progress_ptr->setGeometry(this->geometry().left() + 50, this->geometry().top() + 20, 300, 25);
    progress_ptr->setWindowModality(Qt::ApplicationModal);

    progress_ptr->show();
    //Показывает прогресс обработки файлов в меню на экране
    //!!!!!!!!!!!
    */

    //Далее - работа с файлами:
    //считывание куска данных, его калибровка,
    //покусочная сортировка (с записью результатов в файл и во вспомогательные массивы)
    //- и после чего использование предварительно обработанных данных в каком-либо методе обработки

    /* Понадобится для работы: округление до сотых долей, из
    http://ru.stackoverflow.com/questions/172120/%D0%9E%D0%BA%D1%80%D1%83%D0%B3%D0%BB%D0%B5%D0%BD%D0%B8%D0%B5-%D1%87%D0%B8%D1%81%D0%BB%D0%B0-%D0%BF%D1%83%D1%82%D0%B5%D0%BC-%D0%BC%D0%B0%D1%82%D0%B5%D0%BC%D0%B0%D1%82%D0%B8%D1%87%D0%B5%D1%81%D0%BA%D0%B8%D1%85-%D0%BE%D0%BF%D0%B5%D1%80%D0%B0%D1%86%D0%B8%D0%B9
    ...для отрицательных чисел каст в int работает не так, как предполагалось. Исправленный с учётом этого вариант:
    digit = ((int)(digit*100 + (digit >= 0 ? 0.5 : -0.5))/100.0; // округление до сотых
    */

    //Вначале определим стартовую точку во времени. Поскольку для неба важно звездное время,
    //определяем стартовую точку именно для него. Для этого возьмем стартовую точку , которую ввели через форму,
    //и найдем ближайшую к ней точку-индекс файла, которая попадает на точку в звездном времени ,
    //кратной 10 секундам.




    //?????????????????????????????????????????????
    float step_in_star_second=20.0;
    double step_for_median_data_in_sec=step_in_star_second/coeff_from_Tutc_to_Tstar;
    //Интервал медианного осреднения - сразу от звездных секунд (в которых задаем изначально величину в 20 секунд)
    //привязали к обычным, временнЫм

    double add_to_deriv=31.0;
    //Добавка 31 секунды к первоначально заявленной порции считываемых секунд данных
    //- поскольку это соответствует DM=2000 (в полной полосе частот 2.5 MHz),
    //хочется именно до этого ограничения работать, считывая дополнительные данные,
    //чтобы их можно было складывать с основными (сдвигаясь от них на дополнительные) при больших дисперсиях

    long tmp_ind,tmp_band,tmp_Canal,tmp_max;


    starTime curStarTime(bsaFile.header.date_begin.mid(6,4).toInt(), bsaFile.header.date_begin.mid(3,2).toInt(),
                         bsaFile.header.date_begin.mid(0,2).toInt(),
                         bsaFile.header.time_begin.mid(0,bsaFile.header.time_begin.length() == 7 ? 1 : 2).toInt(),
                         0.0, 0.0, 2.50875326667, 0);
    //starTime - это класс, вычисляющий звездное время для предложенных параметров (см. qbsa.h) - здесь для начала файла
    double star_time_in_Hour_start_file=curStarTime.hour + curStarTime.minute/60.0 + curStarTime.second/3600.0;
    //double star_time_in_Hour_start_file=curStarTime.hour + curStarTime.minute/60 ... - это неверно!
    //Добавка считается тогда как ЦЕЛОЕ число!!!
    double star_time_start_first_H=star_time_in_Hour_start_file+(start_time_in_sec/3600.0 * coeff_from_Tutc_to_Tstar);
     //высчитали звездное время начала обрабатываемого участка (с заданным сдвигом во времени (обычном) от начала файла)
    /*
    while (star_time_start_first_H > 24.0)
      star_time_start_first_H = star_time_start_first_H -24.0;
    //Предусмотрели и поправили возможный переход через звездную полночь НО! Потом опять отнимается, потом сравнивается... Убрал
    */


    double tmp=star_time_start_first_H-int(star_time_start_first_H);
    //tmp=int (tmp/(step_for_median_data_in_sec/3600.0) +0.5);  //НЕВЕРНО - делил на шаг во временных секундах, а надо - в звездных!
    tmp=int (tmp/(step_in_star_second/3600.0) +0.5);
        //нашли ближайшее целое число кусков по 20 секунд в дробной части часов
    //double star_time_start_end_H=int(star_time_start_first_H)+tmp*(step_for_median_data_in_sec/3600.0)
    //           -(step_for_median_data_in_sec/2)/3600.0;
      //НЕВЕРНО!! Вновь использовали временные секунды вместо звездных!
    double star_time_start_end_H=int(star_time_start_first_H)+tmp*(step_in_star_second/3600.0)
               -(step_in_star_second/2)/3600.0;
      //сдвинулись на начало 20 секундного (ЗВЕЗДНЫХ секунд!) отрезка данных
      //минус еще 10 секунд (чтобы метка 20 сек. попадала в центр)

    while (star_time_start_end_H < star_time_start_first_H)
      //star_time_start_end_H = star_time_start_end_H + (add_to_deriv/3600.0);  //НЕВЕРНО! ПРи чем тут add_to_deriv???
      star_time_start_end_H = star_time_start_end_H + (step_in_star_second/3600.0);
    //Хотим априори первую точку старта (окургленную по 20 секундам) расположить после от первоначально определенной
    //ВНИМАНИЕ!!!
    //star_time_start_end_H = star_time_start_end_H + (20/3600);
    //- НЕВЕРНО, поскольку (20/3600)==0 !! Надо:
    //star_time_start_end_H = star_time_start_end_H + (20.0/3600.0);


    double start_time_in_sec_end=(star_time_start_end_H-star_time_in_Hour_start_file)*3600.0/coeff_from_Tutc_to_Tstar;
    long start_point =int( start_time_in_sec_end / (0.001 * bsaFile.header.tresolution) +0.5);
            //Стартовая точка чтения данных файла, столько надо пропустить!
            // устанавливаем позицию на НУЖНОЕ начало двоичных данных
            //- в соответствии со сдвигом начала чтения файла на start_point временнЫх точек
    int counter_median_point_in_deriv=0;
    long ind_in_step_for_median_data=int( step_for_median_data_in_sec / (0.001 * bsaFile.header.tresolution) +0.5) ;
    //????????????????????????????????????????????



    double x1, x2, y1, y2;


    stopThread = false;

    otladka=1;
    int param_1_otladka=4800*4;
    //раз в сколько точек выводим в лог

    if (otladka==1)
      {
        if(fileOut.open(QIODevice::WriteOnly | QIODevice::Text))
          { // Если файл успешно открыт для записи в текстовом режиме

                writeStream.setCodec(QTextCodec::codecForName("cp1251"));
                writeStream << "Start. Work with algorithms find dispersion.\r\n";
                QDate date = QDate::currentDate();
                QTime time = QTime::currentTime();
                str1 = date.toString("dd.MM.yyyy ")+time.toString("hh:mm:ss.zzz");
                writeStream << str1 ; //Запишет в файл системное время
                  // Посылаем строку в поток для записи

                writeStream << "\n start_point_file,ind_in_step_for_median_data="
                            << start_point << ","
                            << ind_in_step_for_median_data << "\n"; //Запишет в файл индекс

                writeStream << "star_time_start_first_H,star_time_start_end_H, start_time_in_sec_end="
                            << star_time_start_first_H << ","
                            << star_time_start_end_H  << ","
                            << start_time_in_sec_end
                            << "\n"; //Запишет в файл индекс

                writeStream << "star_time_in_Hour_start_file, RA: = "
                            << star_time_in_Hour_start_file  << ","
                            << curStarTime.hour << ":"
                            << curStarTime.minute  << ":"
                            << curStarTime.second
                            << "\n"; //Запишет в файл индекс

                fileOut.flush();
                // fileOut.close(); // Закрываем файл
          };//if(fileOut.open(QIODevice::WriteOnly | QIODevice::Text))
       };//if (otladka==1)


    //НАЧАЛО Находим-Запоминаем в переменные общие данные для импульсов:
    //std::string name_file,name_file_short;   ++, ++
    //int device,n_bands,resolution    ++, ++, ++

    name_file=bsaFile.name_file_of_datas.toStdString().c_str();

    const char separator[]=":/\\";
    //char *S = new char[bsaFile.name_file_of_datas.toStdString().Length()];
    char *S = new char[name_file.size()];
    //char S[name_file.size()]="";  - не работает
    strcpy( S, name_file.c_str() );
    //S[name_file.size()]=name_file;
    //S=name_file.c_str();
    //S = bsaFile.name_file_of_datas.toStdString().c_str();
    char *Ptr=NULL;
    Ptr=strtok(S,separator); //Исходная строка будет изменена
    while (Ptr)
    {
       name_file_short=Ptr; //выводим очередную часть строки из строки в искомую переменную
       Ptr=strtok(0,separator); //указываем на новый токен
    }

    device = (bsaFile.name_file_of_datas.indexOf("_n2_", 0, Qt::CaseInsensitive) != -1) ? 2 : 1;
    //(Условие) ? (выражение1):(выражение2)
    // Если условие истинно, то выполняется выражение1, а если ложно, то выражение2.
    // Берем либо 2-ю стойку (если в названии есть "_n2_"), либо по умолчанию 1-ю.
    resolution=bsaFile.header.tresolution;
    n_bands=bsaFile.header.nbands;
    //КОНЕЦ

    //???????????????????????????????????????????????
    unsigned int max_i = ui->spinBox_2->value(); // количество порций считали из формы
    //double deriv_star = (deriv + step_for_median_data_in_sec) / coeff_from_Tutc_to_Tstar;
    //???????????????????????????????????????????????

    double deriv_star = deriv  / coeff_from_Tutc_to_Tstar;
    // количество сдвиговых секунд,вычисленных из ЗВЕЗДНЫХ секунд в порции данных
    //- реально их должно быть меньше, чтобы не было разрывов в данных



//+++++++++++++++Начало обработки - прохода по заданным временным отрезкам+++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/**/
    if (otladka==1)
     {
        writeStream << "\n DO base_processing_of_file : file_or_list,limit_Alpha_min,limit_Alpha_max="
                    << read_from_file_or_list_names_of_files << " , "
                    << limit_Alpha_min << ", "
                    << limit_Alpha_max << ", " << name_file_of_ini  <<
                     "\n"; //Запишет в файл индекс
        fileOut.flush();
     };//if (otladka==1)

    base_processing_of_file(max_i,deriv,step_in_star_second,start_point,start_time_in_sec_end,
                            star_time_in_Hour_start_file,star_time_start_end_H,param_1_otladka,
                                  //раз в сколько точек выводим в лог)
                            do_dispers,limit_dispersion_min,limit_dispersion_max,limit_myltiply,limit_sigma,
                            multiply_y,scale_T,add_multiply,out_data_dir,dispers_data_file,limit_Alpha_min,limit_Alpha_max,
                            limit_Delta_min,limit_Delta_max,limit_UT_min,limit_UT_max,read_from_file_or_list_names_of_files,
                            list_names_of_files,names_of_file_EQ);

 /**/

    /*********                  ***************/


    /*********************************/
    /********* ***************/


//+++++++++++++++КОНЕЦ обработки - прохода по заданным временным отрезкам+++++++++++++++++++++++++++++++++++++++++
    if (otladka==1)
      {
        fileOut.flush();
        fileOut.close(); // Закрываем файл
      };//if (otladka==1)


    //ОКОНЧАНИЕ работы с файлами

    //Концовка работы, совершение завершающе-закрывающих действий

    QDateTime finish = QDateTime::currentDateTime();
    //if(!ui->checkBox_2->isChecked())
        QMessageBox::information(this, "Сообщение", "Время " + QString::number(finish.secsTo(start_time_program)) + " сек ");

    inClientFile.close();
    // Закрыли файл с данными

    //!!!  progress_ptr->hide();
    //Закрыли меню на экране

}

