//***************************************************************************
// Copyright 2007-2020 Universidade do Porto - Faculdade de Engenharia      *
// Laboratório de Sistemas e Tecnologia Subaquática (LSTS)                  *
//***************************************************************************
// This file is part of DUNE: Unified Navigation Environment.               *
//                                                                          *
// Commercial Licence Usage                                                 *
// Licencees holding valid commercial DUNE licences may use this file in    *
// accordance with the commercial licence agreement provided with the       *
// Software or, alternatively, in accordance with the terms contained in a  *
// written agreement between you and Faculdade de Engenharia da             *
// Universidade do Porto. For licensing terms, conditions, and further      *
// information contact lsts@fe.up.pt.                                       *
//                                                                          *
// Modified European Union Public Licence - EUPL v.1.1 Usage                *
// Alternatively, this file may be used under the terms of the Modified     *
// EUPL, Version 1.1 only (the "Licence"), appearing in the file LICENCE.md *
// included in the packaging of this file. You may not use this work        *
// except in compliance with the Licence. Unless required by applicable     *
// law or agreed to in writing, software distributed under the Licence is   *
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF     *
// ANY KIND, either express or implied. See the Licence for the specific    *
// language governing permissions and limitations at                        *
// https://github.com/LSTS/dune/blob/master/LICENCE.md and                  *
// http://ec.europa.eu/idabc/eupl.html.                                     *
//***************************************************************************
// Author: Eivind Jølsgard                                                 *
//***************************************************************************

// ISO C++ 98 headers.
#include <cstddef>
// DUNE headers.
#include <DUNE/DUNE.hpp>
#include <mqtt/async_client.h>



namespace Transports
{
  //! Insert short task description here.
  //!
  //! Insert explanation on task behaviour here.
  //! @author Eivind Jølsgard
  namespace MQTT
  {
    namespace SLIMMessageBridge
    {
      using DUNE_NAMESPACES;

	    mqtt::connect_options conn_opts;
      mqtt::async_client* mqtt_client;
      auto timeout = std::chrono::seconds(10);
      
      //TODO could be made more C++ styled
      typedef struct slim_bouy_status {
        uint16_t  tbr_sensor_id;
        int  latitude;
        int  longitude;
      } slim_bouy_status_t;

      
      slim_bouy_status_t *slim_bouys;
      int number_of_detected_slim_bouys = 0;

       struct Arguments
      {
        std::string address; // Server address.
        std::string mqtt_username;
        std::string mqtt_password;
        int port; // Server port.
        std::string subscribe_topic;  //Topic to subscribe
        std::string publish_topic;  //Topic to publish
        std::string client_id;
        std::string lwt_payload; //Last will testament payload
        int  QOS; //Quality of service
        unsigned int source;  //IMC Source
        unsigned int sourceEntity;  //IMC Source
        unsigned int destination;  //IMC Source
        unsigned int destinationEntity;  //IMC Source
        std::string planId; //IMC Plan ID
        unsigned int planOp; //IMC Plan Op
        unsigned int max_number_of_slim_bouys; 
      };

      struct Task: public DUNE::Tasks::Task
      {
        // Task arguments.
        Arguments m_args;
        // Parser handle.
        IMC::Parser m_parser;

        //Buffers
        DUNE::Utils::ByteBuffer m_buf;
        DUNE::Utils::ByteBuffer m_buf_receive;

        //! Constructor.
        //! @param[in] name task name.
        //! @param[in] ctx context.
        Task(const std::string& name, Tasks::Context& ctx):
          DUNE::Tasks::Task(name, ctx)
        {

          param("MQTT Broker Address", m_args.address)
          .defaultValue("tcp://localhost")
          .description("MQTT broker address");

          param("MQTT Username", m_args.mqtt_username)
          .defaultValue("user")
          .description("MQTT username");

          param("MQTT Password", m_args.mqtt_password)
          .defaultValue("password")
          .description("MQTT password");

          param("Address", m_args.address)
          .defaultValue("tcp://localhost")
          .description("MQTT broker address");

          param("Subscribe Topic", m_args.subscribe_topic)
          .defaultValue("toServer")
          .description("MQTT Subscribe topic");

          param("Port", m_args.port)
          .defaultValue("2023")
          .description("MQTT broker port");

          param("Publish Topic", m_args.publish_topic)
          .defaultValue("defaultPublishTopic")
          .description("MQTT Publish topic");

          param("ClientID", m_args.client_id)
          .defaultValue("Default client ID")
          .description("MQTT Client ID");

          param("LWT Payload", m_args.lwt_payload)
          .defaultValue("LWT payload")
          .description("MQTT Last Will Testament");

          param("QOS", m_args.QOS)
          .defaultValue("1")
          .description("MQTT Quality of service");

          param("IMC Source", m_args.source)
          .defaultValue("65535")
          .description("IMC Source");

          param("IMC Source Entity", m_args.sourceEntity)
          .defaultValue("255")
          .description("IMC Source Entity");

          param("IMC Destination", m_args.destination)
          .defaultValue("26")
          .description("IMC Destination");

          param("IMC Destination Entity", m_args.destinationEntity)
          .defaultValue("46")
          .description("IMC Destination Entity");

          param("IMC PlanID", m_args.planId)
          .defaultValue("s")
          .description("IMC PlanID to start");

          param("IMC PlanOP", m_args.planOp)
          .defaultValue("0")
          .description("IMC PlanOperation");

          param("Max Number Of Bouys", m_args.max_number_of_slim_bouys)
          .defaultValue("1000")
          .description("Number of SLIM Bouys");
        }

        //! Update internal state with new parameter values.
        void
        onUpdateParameters(void)
        {
        }

        //! Reserve entity identifiers.
        void
        onEntityReservation(void)
        {
        }

        //! Resolve entity names.
        void
        onEntityResolution(void)
        {
        }

        //! Acquire resources.
        void
        onResourceAcquisition(void)
        {
          inf(DTR("connecting to MQTT broker"));
          std::string adr_string(m_args.address); 

          adr_string.append(":");
          adr_string.append(std::to_string(m_args.port));


          //inf(adr_string.c_str());

          slim_bouys = new slim_bouy_status_t[m_args.max_number_of_slim_bouys];


          mqtt_client = new mqtt::async_client(adr_string, m_args.client_id);

          conn_opts.set_keep_alive_interval(200); 
	        conn_opts.set_clean_session(true); 
          
          conn_opts.set_user_name(m_args.mqtt_username.c_str());
          conn_opts.set_password(m_args.mqtt_password.c_str());

          try
          {
            mqtt_client->connect(conn_opts)->wait();
            inf(DTR("connected to MQTT broker"));
		        mqtt_client->start_consuming();
            std::string subscribe_str(m_args.subscribe_topic.c_str());
            
            char lastChar = subscribe_str.back();
            if(lastChar == '/')
            {
               subscribe_str.append("#");
            }
             
            
            mqtt_client->subscribe(subscribe_str, m_args.QOS)->wait();
            //inf(subscribe_str);
            
            setEntityState(IMC::EntityState::ESTA_NORMAL, Status::CODE_ACTIVE);
          }
          catch (std::runtime_error& e)
          {
            throw RestartNeeded(e.what(), 5);
          }
          return;
        }

        //! Initialize resources.
        void
        onResourceInitialization(void)
        {
        }

        //! Release resources.
        void
        onResourceRelease(void)
        {
          inf(DTR("onResourceRelease"));
          
          delete slim_bouys;

          try{
              if (mqtt_client)
              {
                //auto toks = mqtt_client->get_pending_delivery_tokens();
               // if (!toks.empty())
               // inf(DTR("Error: There are pending MQTT delivery tokens!" ));
                
                mqtt_client->unsubscribe(m_args.subscribe_topic)->wait();
                mqtt_client->stop_consuming();
                mqtt_client->disconnect()->wait();
                delete mqtt_client;
              }
          }
          catch (std::runtime_error& e)
          {
            throw RestartNeeded(e.what(), 5);
          }
        }




        int setTBRlocation(int tbr_sensor, int latitude, int longitude)
        {
          int sensor_index = -1; 
          for(int i = 0; i < number_of_detected_slim_bouys; i++)
          {
            if(slim_bouys[i].tbr_sensor_id == tbr_sensor)
            {
              sensor_index = i;
              break;
            }
          }
          if(sensor_index == -1) //New sensor
          {
            inf(DTR("adding sensor" ));
            sensor_index = number_of_detected_slim_bouys;
            number_of_detected_slim_bouys++;
          }
          slim_bouys[sensor_index].longitude = longitude;
          slim_bouys[sensor_index].latitude = latitude;
          slim_bouys[sensor_index].tbr_sensor_id = tbr_sensor;
          return 0; 
        }

        int getTBRlocation(int tbr_sensor, int* latitude, int* longitude)
        {
          int sensor_index = -1; 
          for(int i = 0; i < number_of_detected_slim_bouys; i++)
          {
            if(slim_bouys[i].tbr_sensor_id == tbr_sensor)
            {
              sensor_index = i;
              break;
            }
          }
          if(sensor_index == -1) //New sensor
          {
             err(DTR("Sensor not found" ));
            return -1;
          }
          *longitude = slim_bouys[sensor_index].longitude;
          *latitude  = slim_bouys[sensor_index].latitude ;
          return 0; 
        }


        int fillTBRSensorMessage(IMC::TBRSensor* sensor_msg, uint8_t* message, uint32_t base_timestamp)
        { 
          int message_size = 8;
          sensor_msg->unix_timestamp = base_timestamp + message[0];
          sensor_msg->temperature = fp32_t((message[2] << 8) | message[3]);
          sensor_msg->avg_noise_level = message[4];
          sensor_msg->peak_noise_level = message[5];
          sensor_msg->recv_listen_freq = message[6];
          sensor_msg->recv_mem_addr = message[7];

          return message_size;
        }

        int fillTBRFishTagMessage(IMC::TBRFishTag* tag_msg, uint8_t* message, uint32_t base_timestamp)
        {
          int message_size = -1;
          tag_msg->unix_timestamp = base_timestamp + message[0];
          tag_msg->trans_protocol = message[1];


        switch (tag_msg->trans_protocol)
        {
        case 0: //R256
          tag_msg->trans_protocol = IMC::TBRFishTag::TBR_R256;
          tag_msg->trans_id       = message[2];
          tag_msg->trans_data     = 0;
          message_size            = 5;
          break;
        case 1://R04K
          tag_msg->trans_protocol = IMC::TBRFishTag::TBR_R04K;
          tag_msg->trans_id       = message[2] << 8 | message[3];
          tag_msg->trans_data     = 0;
          message_size            = 6;
          break;
        case 2://R64K
          tag_msg->trans_protocol = IMC::TBRFishTag::TBR_R64K;
          tag_msg->trans_id       = message[2] << 8 | message[3];
          tag_msg->trans_data     = 0;
          message_size            = 6;
          
          break;
        case 3: //S256
          tag_msg->trans_protocol = IMC::TBRFishTag::TBR_S256;
          tag_msg->trans_id       = message[2];
          tag_msg->trans_data     = message[3];
          message_size            = 6;
          
          break;
        case 4: //R01M
          tag_msg->trans_protocol = IMC::TBRFishTag::TBR_R01M;
          tag_msg->trans_id       = message[2] << 8 | message[3];
          tag_msg->trans_data     = message[4];
          message_size            = 7;
          
          break;
        case 5: //S64K
          tag_msg->trans_protocol = IMC::TBRFishTag::TBR_S64K;
          tag_msg->trans_id       = message[2] << 8 | message[3];
          tag_msg->trans_data     = message[4];
          message_size            = 7;
          break;
        case 6: //HS256
          tag_msg->trans_protocol = IMC::TBRFishTag::TBR_HS256;
          tag_msg->trans_id       = message[2];
          tag_msg->trans_data     = message[3] << 8 | message[4];
          message_size            = 7;
          break;
        case 7: //DS256
          tag_msg->trans_protocol = IMC::TBRFishTag::TBR_DS256;
          tag_msg->trans_id       = message[2];
          tag_msg->trans_data     = message[3] << 8 | message[4];
          message_size            = 7;
          break;
        default: 
          //Unknown protocol
          return -1;
          break;
        }

          tag_msg->snr            = message[message_size -2] >>2;
          tag_msg->millis         = (message[message_size -2] & 0b11) << 8 | message[message_size -1];
          tag_msg->trans_freq     = 0; //Not part of SLIM message format, this has to be adapted if detection frequency
          tag_msg->recv_mem_addr  = 0; // and recv_mem_addr is to be added.

          return message_size;
        }
        

        //! Main loop.
        void
        onMain(void)
        {
            //Variable declarations
            uint8_t* p_receive;
            unsigned int n_r_receive = 0;

          while (!stopping())
          {
            
            waitForMessages(1.0);

            //Extract new message
            auto msg = mqtt_client->consume_message();
			      if (msg)
            {
            inf(DTR("Receiving message" ));
              
                

              try
              {
                //Make sure the topic is the requested topic
                std::string topic = msg->get_topic();
                std::string part_of_topic_to_compare = topic.substr(0, m_args.subscribe_topic.size() - 1);

                if(part_of_topic_to_compare.compare(m_args.subscribe_topic))
                {

                  n_r_receive = msg->get_payload().size();
                  
                  p_receive = (uint8_t*) msg->get_payload().data();


                  
                  if (n_r_receive > 6) //Message must at least contain SLIM header
                  //extract message
                  {
                    inf(DTR("Extract header" ));
                    //extract header
                    int tbr_serial_number = (p_receive[0] << 6) | (p_receive[1] >> 2);
                    int reference_timestamp = p_receive[2]  << 24 |
                                                p_receive[3]  << 16 |
                                                p_receive[4]  <<  8 |
                                                p_receive[5]; 
                    int header_flag = p_receive[1] & 0b11;

                    if(header_flag == 0) //TBR message
                    {              
                      inf(DTR("Got TBR message" )); 
                      //get position 
                      int latitude = 0; 
                      int longitude = 0; 
                      getTBRlocation(tbr_serial_number, &latitude, &longitude);  //assume bouy is located at same position as last status message, else use (0,0)
                      if(latitude == 0 && longitude == 0)
                      {
                        err(DTR("Location of TBR sensor not known, sending with coordinates (lat:0, long:0)" )); 
                      }
                      
                      //extract messages
                      unsigned int message_index = 6;
                      IMC::TBRSensor sensor_msg;
                      sensor_msg.serial_no = tbr_serial_number;


                      IMC::TBRFishTag fish_tag_msg;
                      fish_tag_msg.serial_no = tbr_serial_number;
                      fish_tag_msg.lat = latitude; 
                      fish_tag_msg.lon = longitude;


                      int frame_size = 0;
                      while(message_index + 5 <= n_r_receive)
                      {
                        inf(DTR("Extracting message" )); 
                        if(p_receive[message_index + 1] == 255) //sensor data frame
                        {
                          frame_size = fillTBRSensorMessage(&sensor_msg, &(p_receive[message_index]), reference_timestamp);
 
                          if(frame_size > 0)
                          {
                            message_index += frame_size;
                            inf(DTR("Dispatch sensor msg" ));
                            dispatch(sensor_msg, DF_KEEP_TIME | DF_KEEP_SRC_EID);
                          }
                          else
                          {
                            //error unpacking message
                            break;
                          }
                          
                        }
                        else
                        {
                          frame_size = fillTBRFishTagMessage(&fish_tag_msg, &(p_receive[message_index]), reference_timestamp);

                          if(frame_size > 0)
                          {
                            message_index += frame_size;
                            fish_tag_msg.toText(std::cout);
                            inf(DTR("Dispatch fish tag msg" ));
                            dispatch(fish_tag_msg, DF_KEEP_TIME | DF_KEEP_SRC_EID);
                          }
                          else
                          {
                            //error unpacking message
                            break;
                          }
                        }
                        
                      }


                    }
                    else if(header_flag == 1) //SLIM status message
                    {
                      inf(DTR("Got SLIM Status message" )); 
                      //extract TBR sensor and position and store such that it can be retrieved when handling TBR messages
                      int latitude = (p_receive[5] & 0b1)<<24 |
                                      p_receive[6] << 16 |
                                      p_receive[7] <<  8 | 
                                      p_receive[8]; 
                      int longitude =(p_receive[1] & 0b11)<<24 |
                                      p_receive[2] << 16 |
                                      p_receive[3] <<  8 | 
                                      p_receive[4]; 
                      setTBRlocation(tbr_serial_number, latitude, longitude);

                    
                    }
                

                  }     


                }
                else{
                    inf(DTR("Topic of no interrest" ));
                }
              }
              catch (std::exception& e)
              {
                  throw RestartNeeded(e.what(), 5);
              }

              

            }
			    
          }
          }
      };
    }
  }
}

DUNE_TASK
