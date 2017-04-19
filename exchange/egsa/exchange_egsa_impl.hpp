#ifndef EXCHANGE_EGSA_IMPL_HPP
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
#include "mdp_worker_api.hpp"
#include "msg_message.hpp"
// Конфигурация
#include "exchange_config.hpp"
#include "exchange_egsa_site.hpp"
#include "exchange_egsa_cycle.hpp"
#include "exchange_egsa_request.hpp"

class ExternalSMAD;

class EGSA : public mdp::mdwrk {
  public:
    enum { BROKER_ITEM = 0, SUBSCRIBER_ITEM = 1, SERVICE_ITEM = 2, SOCKET_MAX_COUNT = SERVICE_ITEM + 1 };
    static const int PollingTimeout;

    EGSA(const std::string&, const std::string&);
   ~EGSA();
    
    // Основной рабочий цикл
    int run();

    //============================================================================
#ifdef _FUNCTIONAL_TEST
    // Если собирается тест, вынести эти функции в публичный доступ
  public:
#else
    // Если собирается рабочая версия, спрятать эти функции в приватный доступ
  private:
#endif
    // Запуск Интерфейса второго уровня
    int implementation();
    // Доступ к конфигурации
    EgsaConfig* config();
    // Доступ к Сайтам
    AcqSiteList& sites();
    // Доступ к Циклам
    CycleList& cycles();
    // Доступ к Запросам
    RequestList& requests();
    // Ввести в оборот новый Цикл сбора
    size_t push_cycle(Cycle*);
    // Активировать циклы
    int activate_cycles();
    // Деактивировать циклы
    int deactivate_cycles();
    // Тестовая функция ожидания срабатывания таймеров в течении заданного времени
    int wait(int);
    // Инициализация, создание/подключение к внутренней SMAD
    int init();
    // Инициализация внутренних массивов
    int load_config();
    // Первичная обработка нового запроса
    int processing(mdp::zmsg*, const std::string&, bool&);
    void tick_tack();
    //============================================================================

  private:
    DISALLOW_COPY_AND_ASSIGN(EGSA);

    // Обработка сообщения о чтении значений БДРВ (включая ответ группы подписки)
    int process_read_response(msg::Letter*);
    // Останов экземпляра
    int stop();
    // Получить новое сообщение
    // Второй параметр - количество милисекунд ожидания получения сообщения.
    // =  0 - разовое чтение сообщений, немедленный выход в случае отсутствия таковых
    // >  0 - время ожидания нового сообщения в милисекундах, от 1 до HEARTBEAT-интервала
    int recv(msg::Letter*&, int = 1000);

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
    int handle_end_all_init(msg::Letter*, const std::string&);
    int handle_asklife(msg::Letter*, const std::string&);
    int handle_stop(msg::Letter*, const std::string&);

    // Отправить всем подчиненным системам запрос готовности
    void fire_ENDALLINIT();

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
    void cycle_trigger(size_t, size_t);

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
    // Хранилище изменивших своё значение параметров, используется для всех СС
    sa_parameters_t m_list;
    // Сокет получения данных
    int m_socket;

    // General data
    //static ega_ega_odm_t_GeneralData ega_ega_odm_r_GeneralData;

    // Acquisition Sites Table
    AcqSiteList m_ega_ega_odm_ar_AcqSites;
	
    // Cyclic Operations Table
    //static ega_ega_odm_t_CycleEntity ega_ega_odm_ar_Cycles[NBCYCLES];
    CycleList   m_ega_ega_odm_ar_Cycles;

    // Request Table - перенёс в exchange_egsa_init.cpp
    //static ega_ega_odm_t_RequestEntry m_requests_table[/*NBREQUESTS*/]; // ega_ega_odm_ar_Requests
    RequestList m_ega_ega_odm_ar_Requests;
    //std::vector<Request*> m_ega_ega_odm_ar_Requests;
};

#endif

