#if !defined EXCHANGE_EGSA_IMPL_HPP
#define EXCHANGE_EGSA_IMPL_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <map>
#include <vector>
#include <thread>

// Служебные заголовочные файлы сторонних утилит
// Служебные файлы RTDBUS
#include "mdp_zmsg.hpp"
#include "mdp_worker_api.hpp"
#include "msg_message.hpp"
// Конфигурация
#include "exchange_config.hpp"
#include "exchange_config_egsa.hpp"
// Внешняя память, под управлением EGSA
#include "exchange_egsa_init.hpp"
//#include "exchange_egsa_sa.hpp"

class ExternalSMAD;
class EgsaConfig;
class SystemAcquisition;
class Cycle;
class Request;

class EGSA : public mdp::mdwrk {
  public:
    enum { BROKER_ITEM = 0, SUBSCRIBER_ITEM = 1, SERVICE_ITEM = 2, SOCKET_MAX_COUNT = SERVICE_ITEM + 1 };
    static const int PollingTimeout;
    typedef std::map<std::string, SystemAcquisition*> system_acquisition_list_t;

    EGSA(const std::string&, const std::string&);
   ~EGSA();
    
    // Основной рабочий цикл
    int run();

    // Получить набор циклов, в которых участвует заданная СС
    std::vector<Cycle*>   *get_Cycles_for_SA(const std::string&);
    // Получить набор запросов, зарегистрированных за данной СС
    std::vector<Request*> *get_Request_for_SA(const std::string&);

    // Ввести в оборот новый Цикл сбора
    int push_cycle(Cycle*);

  private:
    DISALLOW_COPY_AND_ASSIGN(EGSA);
    
    // Запуск Интерфейса второго уровня
    int implementation();

    int handle_asklife(msg::Letter*, const std::string&);
    int handle_stop(msg::Letter*, const std::string&);

    // Первичная обработка нового запроса
    int processing(mdp::zmsg*, const std::string&, bool&);
    // Обработка сообщения о чтении значений БДРВ (включая ответ группы подписки)
    int process_read_response(msg::Letter*);
    // Инициализация, создание/подключение к внутренней SMAD
    int init();
    // Останов экземпляра
    int stop();
    // Получить новое сообщение
    // Второй параметр - количество милисекунд ожидания получения сообщения.
    // =  0 - разовое чтение сообщений, немедленный выход в случае отсутствия таковых
    // >  0 - время ожидания нового сообщения в милисекундах, от 1 до HEARTBEAT-интервала
    int recv(msg::Letter*&, int = 1000);
    // Активировать циклы
    int activate_cycles();
    // Деактивировать циклы
    int deactivate_cycles();
    // Тестовая функция ожидания срабатывания таймеров в течении заданного времени
    int wait(int);
    // Доступ к циклам
    const std::vector<Cycle*>& cycles() { return ega_ega_odm_ar_Cycles; };

    // Отправить всем подчиненным системам запрос готовности
    void fire_ENDALLINIT();
    // --------------------------------------------------------------------------
    // Обслуживание запросов:
    // SIG_D_MSG_READ_MULTI - ответ на множественное чтение данных (первый пакет от группы подписки)
    int handle_read_multiple(msg::Letter*, const std::string&);
    // SIG_D_MSG_GRPSBS - порция обновления данных от группы подписки
    int handle_sbs_update(msg::Letter*, const std::string&);
    // Телерегулирование
    // SIG_D_MSG_ECHCTLPRESS
    // SIG_D_MSG_ECHDIRCTLPRESS
    int handle_teleregulation(msg::Letter*, const std::string&);
    // --------------------------------------------------------------------------
    // Активация группы подписки точек систем сбора 
    int activateSBS();
    // Дождаться ответа на запрос активации группы подписки
    int waitSBS();
    // Подключиться к SMAD систем сбора
    int attach_to_sites_smad();
    // Изменение состояния подключенных систем сбора и отключение от их внутренней SMAD 
    int detach();
    // Функция срабатывания при наступлении времени очередного таймера
    static int trigger();

    // Входящее соединение от Таймеров
    zmq::socket_t   m_signal_socket;
    // Набор для zmq::poll
    //zmq::pollitem_t m_socket_items[2];
    msg::MessageFactory *m_message_factory;
    // Сокет обмена сообщениями с Интерфейсом второго уровня
    zmq::socket_t   m_backend_socket;
    // Нить подчиненного интерфейса
    std::thread*    m_servant_thread;


    // Экземпляр ExternalSMAD для хранения конфигурации и данных EGSA
    ExternalSMAD *m_ext_smad;
    EgsaConfig   *m_egsa_config;
    system_acquisition_list_t m_sa_list;
    // Хранилище изменивших своё значение параметров, используется для всех СС
    sa_parameters_t m_list;
    // Сокет получения данных
    int m_socket;

    // General data
    //static ega_ega_odm_t_GeneralData ega_ega_odm_r_GeneralData;

    // Acquisition Sites Table
    //static ega_ega_odm_t_AcqSiteEntry ega_ega_odm_ar_AcqSites[ECH_D_MAXNBSITES];
	
    // Cyclic Operations Table
    //static ega_ega_odm_t_CycleEntity ega_ega_odm_ar_Cycles[NBCYCLES];
    std::vector<Cycle*> ega_ega_odm_ar_Cycles;

    // Request Table - перенёс в exchange_egsa_init.cpp
    //static ega_ega_odm_t_RequestEntry m_requests_table[/*NBREQUESTS*/]; // ega_ega_odm_ar_Requests
    std::vector<Request*> ega_ega_odm_ar_Requests;
};

#endif

