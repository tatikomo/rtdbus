#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_config_sac.hpp"
#include "exchange_smad.hpp"
#include "exchange_sysacq_intf.hpp"

SysAcqInterface::SysAcqInterface(const std::string& _name,
                                 zmq::context_t& _ctx,
                                 AcquisitionSystemConfig& _cfg)
  :
  m_status(STATUS_OK_NOT_CONNECTED),
  m_config(_cfg),
  m_sa_common(),
  m_smad(NULL),
  m_service_name(_name),
  m_connection_restored(0),
  m_num_connection_try(0),
  m_zmq_context(_ctx)
{
  const char* fname = "SysAcqInterface()";

  // Конфигурационные файлы в порядке?
  if (OK == m_config.load())
  {
    // Создать SMAD из указанного снимка
    m_smad = new SMAD(m_config.name().c_str(),
                      m_config.nature(),
                      m_config.smad_filename().c_str());

    // Открыть SMAD
    if (STATE_OK != m_smad->connect())
    {
      LOG(ERROR) << fname << ": Unable to continue without SMAD " << m_config.name();
      m_status = STATUS_FATAL_SMAD;
      // NB: m_smad будет удалён в деструкторе
    }

    // Создать сокет обмена с системно-зависимой частью модуля,
    // предназначеный для двустороннего обмена данными и командами
    // с фронтендом Системы Сбора
    m_zmq_frontend = new zmq::socket_t(m_zmq_context, ZMQ_PAIR);
    m_zmq_frontend->bind(s_SA_INTERNAL_EXCHANGE_ENDPOINT);
  }
  else
  {
    LOG(ERROR) << fname << " : reading configuration files";
    m_status = STATUS_FATAL_CONFIG;
  }
};

// --------------------------------------------------------------------------------
SysAcqInterface::~SysAcqInterface()
{
  LOG(INFO) << "Destroy common SA interface: " << m_service_name;
  delete m_zmq_frontend;
  delete m_smad;
}

// --------------------------------------------------------------------------------
// Функция обработки всех запросов, полученных от верхнего уровня,
// которые не перехватываются на верхнем уровне
int SysAcqInterface::handle_request(mdp::zmsg*, std::string*& reply_to)
{
  const char* fname = "SysAcqInterface::handle_request";
  int rc = NOK;

  return rc;
}


#if 0
// --------------------------------------------------------------------------------
// Запрос
int SysAcqInterface::handle_simple_response(msg::Letter* letter, std::string* reply_to, int status)
{
  int rc = OK;
  const rtdbMsgType usr_request_type = letter->header()->usr_msg_type();
  msg::SimpleReply *simple_reply = NULL;
  rtdbMsgType usr_reply_type;

  switch (usr_request_type) {
     //case ADG_D_MSG_LIVING   : - уже обрабатывается в handle_asklife()
     case ADG_D_MSG_ENDALLINIT : usr_reply_type = ADG_D_MSG_EXECRESULT; break;
     case ADG_D_MSG_ENDALLINIT : usr_reply_type = ADG_D_MSG_ENDINITACK; break;
     case ECH_D_MSG_INT_REQUEST: usr_reply_type = ECH_D_MSG_INT_REPLY;  break;
       LOG(INFO) << "Got usr msg type #" << usr_request_type << ", send msg type #" << usr_reply_type;
       simple_reply = static_cast<msg::SimpleReply*>(m_message_factory->create(usr_reply_type));
       break;

      default :
        rc = NOK;
  }

  send_to_broker((char*) MDPW_REPORT, NULL, response);

  delete response;
  delete simple_reply;

  return rc;
}
#endif



