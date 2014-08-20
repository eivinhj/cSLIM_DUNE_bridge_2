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

#ifndef MANEUVER_MULTIPLEXER_DISLODGE_HPP_INCLUDED_
#define MANEUVER_MULTIPLEXER_DISLODGE_HPP_INCLUDED_

#include <DUNE/DUNE.hpp>

using DUNE_NAMESPACES;

namespace Maneuver
{
  namespace Multiplexer
  {
    // Export DLL Symbol.
    class DUNE_DLL_SYM Dislodge;

    //! Dislodge maneuver
    class Dislodge
    {
    public:
      //! Default constructor.
      //! @param[in] task pointer to Maneuver task
      Dislodge(Maneuvers::Maneuver* task):
        m_task(task)
      { }

      //! Start maneuver function
      //! @param[in] maneuver idle maneuver message
      void
      start(const IMC::Dislodge* maneuver)
      {
        m_task->setControl(IMC::CL_SPEED);

        m_rpm = maneuver->rpm;
        m_dir = maneuver->direction;
      }

      //! On EstimatedState message
      //! @param[in] msg EstimatedState message
      void
      onEstimatedState(const IMC::EstimatedState* msg)
      {
        m_estate = *msg;
      }

      ~Dislodge(void)
      { }

    private:
      enum State
      {
        //! Starting state
        ST_START,
        //! Going backwards
        ST_BACK,
        //! Going forward
        ST_FRONT,
        //! Check results
        ST_CHECK_RESULTS,
        //! Failed state
        ST_FAILED
      };

      //! EstimatedState data
      IMC::EstimatedState m_estate;
      //! Speed in rpms
      float m_rpm;
      //! Direction for dislodge
      uint8_t m_dir;
      //! Pointer to task
      Maneuvers::Maneuver* m_task;
    };
  }
}

#endif
