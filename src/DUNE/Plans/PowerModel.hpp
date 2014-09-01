//***************************************************************************
// Copyright 2007-2014 Universidade do Porto - Faculdade de Engenharia      *
// Laboratório de Sistemas e Tecnologia Subaquática (LSTS)                  *
//***************************************************************************
// This file is part of DUNE: Unified Navigation Environment.               *
//                                                                          *
// Commercial Licence Usage                                                 *
// Licencees holding valid commercial DUNE licences may use this file in    *
// accordance with the commercial licence agreement provided with the       *
// Software or, alternatively, in accordance with the terms contained in a  *
// written agreement between you and Universidade do Porto. For licensing   *
// terms, conditions, and further information contact lsts@fe.up.pt.        *
//                                                                          *
// European Union Public Licence - EUPL v.1.1 Usage                         *
// Alternatively, this file may be used under the terms of the EUPL,        *
// Version 1.1 only (the "Licence"), appearing in the file LICENCE.md       *
// included in the packaging of this file. You may not use this work        *
// except in compliance with the Licence. Unless required by applicable     *
// law or agreed to in writing, software distributed under the Licence is   *
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF     *
// ANY KIND, either express or implied. See the Licence for the specific    *
// language governing permissions and limitations at                        *
// https://www.lsts.pt/dune/licence.                                        *
//***************************************************************************
// Author: Pedro Calado                                                     *
//***************************************************************************

#ifndef DUNE_PLANS_POWERMODEL_HPP_INCLUDED_
#define DUNE_PLANS_POWERMODEL_HPP_INCLUDED_

//! ISO C++ headers.
#include <vector>
#include <map>

// DUNE headers.
#include <DUNE/IMC.hpp>
#include <DUNE/Parsers/Config.hpp>
#include <DUNE/Math/General.hpp>

namespace DUNE
{
  namespace Plans
  {
    // Export DLL Symbol.
    class DUNE_DLL_SYM PowerModel;

    //! Maximum number of payloads
    static const unsigned c_max_payloads = 10;

    //! Utility class to compute offline power conversions and hold model parameters.
    //! Consider ONLY positive speed.
    class PowerModel
    {
    public:
      //! Constructor
      //! @param[in] cfg reference to Config parser
      PowerModel(Parsers::Config* cfg)
      {
        std::string sec = "General";

        cfg->get(sec, "Power Model -- Conversion - Watt", "50.0", m_conv_watt);
        cfg->get(sec, "Power Model -- Conversion - RPM", "1000.0", m_conv_rpm);
        cfg->get(sec, "Power Model -- Hotel Load", "40.0", m_hotel_load);

        for (unsigned i = 0; i < c_max_payloads; ++i)
        {
          std::string option = Utils::String::str("Power Model -- Payload%u - Label", i);
          std::string label;
          cfg->get(sec, option, "", label);

          if (label.empty())
            break;

          option = Utils::String::str("Power Model -- Payload%u - Power", i);
          float power;
          cfg->get(sec, option, "", power);

          std::pair<std::string, float> pl(label, power);
          m_payloads.insert(pl);
        }
      }

      //! Validate the model
      void
      validate(void) const
      {
        if (!m_conv_watt.size() || !m_conv_rpm.size())
          throw std::runtime_error("power model has empty parameters");

        if (m_conv_watt.size() != m_conv_rpm.size())
          throw std::runtime_error("power model sizes do not match");
      }

      //! Compute energy consumed by motor for some RPM value
      //! @param[in] rpm value of rpms to convert from
      //! @param[in] duration amount of time rotating at rpm
      //! @return energy consumed in Wh
      float
      computeMotionEnergy(float rpm, float duration) const
      {
        if (rpm <= 0.0f || duration <= 0.0f)
          return 0.0;

        float power;

        if (m_conv_watt.size() == 1)
          power = rpm * m_conv_watt[0] / m_conv_rpm[0];
        else
          power = Math::piecewiseLI(m_conv_watt , m_conv_rpm, rpm);

        return power * duration / 3600.0;
      }

      //! Compute energy consumed by a payload entity
      //! @param[in] label name of the payload
      //! @param[in] duration amount of time active
      //! @return energy consumed in Wh
      float
      computePayloadEnergy(const std::string& label, float duration) const
      {
        if (!m_payloads.size())
          return 0.0;

        std::map<std::string, float>::const_iterator itr;
        itr = m_payloads.find(label);
        if (itr == m_payloads.end())
          return 0.0;

        return itr->second * duration / 3600.0;
      }

      //! Compute energy consumed by minimal resources
      //! @param[in] duration amount of time in seconds
      //! @return energy consumed in Wh
      float
      computeHotelEnergy(float duration) const
      {
        return m_hotel_load * duration / 3600.0;
      }

    private:
      //! Conversion values for power (Watt)
      std::vector<float> m_conv_watt;
      //! Conversion values for speed (rpm)
      std::vector<float> m_conv_rpm;
      //! Hotel load of the model
      float m_hotel_load;
      //! Map of payloads to power consumed
      std::map<std::string, float> m_payloads;
    };
  }
}

#endif
