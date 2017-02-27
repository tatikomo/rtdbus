//
// Класс для взаимных трансформаций "тип СС" <-> "название СС"
//
#ifndef EXCHANGE_CONFIG_NATURE_H
#define EXCHANGE_CONFIG_NATURE_H
#pragma once

#include "exchange_config.hpp"

class SacNature {
  public:
    static gof_t_SacNature get_natureid_by_name(const std::string& nature_name) {
      gof_t_SacNature nature_id = GOF_D_SAC_NATURE_EUNK;

      for(int i=GOF_D_SAC_NATURE_DIR; i < GOF_D_SAC_NATURE_EUNK; i++)
        if (0 == m_sa_nature_dict[i].designation.compare(nature_name)) {
          nature_id = m_sa_nature_dict[i].nature_code;
          break;
        }

      return nature_id;
    };

    static const std::string& get_naturename_by_id(gof_t_SacNature nature_id) {
      return m_sa_nature_dict[nature_id].designation;
    };

  private:
    SacNature() {};
    // Связь между символьным и числовым кодами типа систем сбора 
    typedef struct {
      gof_t_SacNature nature_code;
      std::string designation;
    } map_nature_dict_t;

    // ---------------------------------------------------------
    static const map_nature_dict_t m_sa_nature_dict[];
};


#endif

