#ifndef GAMMA_H
#define GAMMA_H

#include <QDialog>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

#include <map>
#include <QIcon>
#include <QProgressDialog>

#include "qbsa.h"
#include "GraphicWidget.h"
#include "qcustomplot.h"

namespace Ui {
class grav;
}

typedef struct{
std::string name_file,name_file_short,name_file_graph,good_or_bad;
  //имя файла данных с директорией, имя только самого файла, имя графического файла (без директории),
  //подпись - хороший или плохой импульс - определяем на основе величины n_rays_more_than_max
int device,n_bands; float resolution; int ray,ind;
double MJD;
QDateTime UT;
float Star_Time, d_T_zona, d_T_Half_H, max_Data, S_to_N, Dispers;
int n_similar_Impuls_in_rays,blended,n_bands_more_than_max;} Point_Impuls_D;

class grav : public QDialog
{
    Q_OBJECT

public:
    explicit grav(QWidget *parent = 0);
    ~grav();

    //data_files cFile;
    qbsa bsaFile;
    double mjd_start, curTime;
    //Глобальные переменные для хранения MJD и текущего времени - начала рисунка
    starTime *pStarTime;  //звездное время, считывается из *pStarTime
    unsigned int npoints; //глобальная переменная для хранения числа точек (временнЫх) в файле данных
                          //- потом она используется как долговременный буфер, а вот
                          // bsaFile.header.npoints = npoints / 3600 * deriv;
                                //- отдается на хранение числа точек файла в данном куске из deriv секунд
    bool stopThread; //логическая переменная (для остановки вывода графики)

    std::map<double, std::vector<double>> kalib278;
    std::map<double, std::vector<double>> kalib2400;
    double total; // для вычисления среднего
    GraphicWidget widget_2;
      //используется для отрисовки графиков с последующим сохранением в файлы

    int otladka;//для отладки программ, если = 1 - пишем в файл отладочную информацию
    QString name_file_of_ini; //имя файл с начальной инициализирющей информацией


private slots:
    void on_pbSelectFile_clicked();
    void on_pushButton_clicked();
    void on_pbSelectHR_clicked();
    void on_pushButton_2_clicked();
    void on_progress_cancel();
    void on_toolButton_3_clicked();

    void chooseFile();
    double calib(double y, double ss, int numberBand, int numberCanal, int otladka_add_1);//калибровка данных
    void runSaveFile();//основная функция для чтения и отрисовки куска файла

    void on_pushButton_3_toggled(bool checked);

    //слоты для работы с графиком
    void selectionChanged();
    void mousePress(QMouseEvent*);
    void mouseWheel(QWheelEvent*);
    void setRange(QCPRange);

    void on_tbHelp_clicked();


    //Пишем файл сжатых данных (по умолчанию по 20 звездных секунд) - после калибровки и нахождения данных по всем лучам :
    void write_files_short_data_from_calb_data(std::string out_data_dir,
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
                                          std::vector<long> max_data_vec_index);

    void runSaveFigure_in_File(int band_or_ray, float add_tmp, double koeff,
                               //const QDateTime now, //время UT начала данных
                               double start_time_in_sec, const long start_point,
                               double multiply_y, double scale_T,
                               double add_multiply,
                               float srednee_all_band,
                               const std::vector<float> &test_vec,
                               std::vector<float> &test_vec_end,
                               const std::vector<float> &test_vec_end_tmp,
                               Point_Impuls_D &Impuls);

    /*
     * Проходим по массиву импульсов, ищем одни и те же на разных лучах
     * (то есть число родственных импульсов поперек лучей - если во всех это помехи):
     */
    void Analyze_and_rewrite_ArrayImpulses(unsigned int countsCanal, std::vector<Point_Impuls_D> &Impulses_vec);

    /*
     * Записываем массив импульсов в файл, по одному импульсу на строку.
     * В дальнейшем считаем эти импульсы в базу.
     */
    void Write_ArrayImpulses_in_file(std::string out_data_dir,
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
                                     std::vector<Point_Impuls_D> &Impulses_vec);


    /*
     *
      Find_Number_good_ray_with_max_for_this_Dispers:
      найдем, во скольких хороших лучах произошло превышение над средним
      максимумом (теперь он в max_Data, после деления на число хороших лучей)
      Если превышение только в 1-2 лучах - это плохо, означает что мы зацепили
      мощную помеху с одного из лучей в развертке по дисперсии сквозь сдвиги по лучам.
      Пометим такой импульс как BAD , записав в импульс число сосчитанных превышений -
      мера "хорошести" импульса. Вероятно, выставить как хотя бы четверть - для 32 - восемь,
      для 6 полос - ставим 2
     */
    /*
    void Find_Number_good_ray_with_max_for_this_Dispers(float Dispers,
                                                                long ind_left_Half_H,
                                                                long ind_righte_Half_H,
                                                                const int size_point,
                                                                const unsigned int currentCanal,
                                                                const unsigned int startBand,
                                                                const unsigned int finishBand,
                                                                int &counter_good_bands,
                                                                std::string &good_or_bad,
                                                        const std::vector<int> &data_file_filter
                                                       //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос
                                                   );
                                                   */
    int Find_Number_good_ray_with_max_for_this_Dispers(long ind_left_Half_H, long ind_righte_Half_H,
                                                       const int size_point,
                                                       const unsigned int currentCanal,
                                                       const unsigned int startBand,
                                                       const unsigned int finishBand,
                                                       float Dispers,
                                                       float max_Data,
                                                       float srednee_all_band,
                                                       const std::vector<int> &data_file_filter
                                                        //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос
                                                    );
    /*(float Dispers,
                                                                long ind_left_Half_H,
                                                                long ind_righte_Half_H,
                                                                const int size_point,
                                                                const unsigned int currentCanal,
                                                                const unsigned int startBand,
                                                                const unsigned int finishBand,
                                                                int &counter_good_bands,
                                                                std::string &good_or_bad,
                                                        //const std::vector<int> &data_file_filter
                                                         //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос
                                                                std::vector<int> data_file_filter
                                                               //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос
                                                              );*/

    /*
     * Функция для вычисления данных в одной точке - в зависимости от значения дисперсии
     */
    double Find_Data_from_Dispers(long index_data,
                                  const int size_point,
                                  const unsigned int currentCanal,
                                  const unsigned int startBand,
                                  const unsigned int finishBand,
                                  float Dispers,
                                  const std::vector<int> &data_file_filter
                                   //маска-фильтр (брать =1, не брать=0) вдоль лучей и полос
                              );

    void FindImpulses_in_Data(int band_or_ray, float add_tmp, double koeff, double start_time_in_sec,
                              double multiply_y, double scale_T, double add_multiply,
                              float srednee_all_band, float sigma_all_band,
                              float limit_sigma_all_band, float limit_sigma,
                              std::string name_file, std::string name_file_short,
                              int device, int n_bands, float resolution, /*int ray,*/ const long start_point,
                              const double mjd_start, const double star_time_in_Hour_start_file, const QDateTime now,
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
                              int cod_otladka);

    // грузим  шапку файла данных в указанную структуру данных:
    void load_head_of_file_with_data(QString name_file_of_bsadata,
                                           qbsa &bsaFile_read);

    // Загрузим файл эквивалентов в соответствующие калибровочные вектора данных
    void load_of_file_equivalents(QString name_file_of_equivalents, unsigned int nCountPoint);

    //Считываем данные в заданных рамках (времен, координат и т.д.) в небольшой кусок данных-
    //на основе последовательного списка файлов данных
    void read_data_from_list_of_files(unsigned int &alarm_cod_of_read,
                                      unsigned int &current_number_of_file_in_list,
                                      QDateTime &current_time_UT,
                                      double &current_time_RA, double &star_time_start_end_H,
                                      double &start_time_in_sec_end, long &start_point,
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
                                      std::string &list_names_of_files);

    //Делаем обработку большого куска данных - считывая данные по маленьким кускам - калибровка, нахождение медианных, поиск дисперсий
    void base_processing_of_file(unsigned int max_i,
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
                                 std::string &names_of_file_EQ);

    void on_pushButton_4_clicked();

    void on_selectIniFile_clicked();

private:
    Ui::grav *ui;
};



//vector<Point_Impuls_D> Impulses_vec;

#endif // GAMMA_H
