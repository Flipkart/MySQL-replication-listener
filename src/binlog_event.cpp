/*
Copyright (c) 2003, 2011, 2013, Oracle and/or its affiliates. All rights
reserved.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of
the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
02110-1301  USA
*/

#include "binlog_event.h"
#include <iostream>
typedef unsigned long ulong;

namespace mysql
{

namespace system {

const char *get_event_type_str(Log_event_type type)
{
  switch(type) {
  case START_EVENT_V3:  return "Start_v3";
  case STOP_EVENT:   return "Stop";
  case QUERY_EVENT:  return "Query";
  case ROTATE_EVENT: return "Rotate";
  case INTVAR_EVENT: return "Intvar";
  case LOAD_EVENT:   return "Load";
  case NEW_LOAD_EVENT:   return "New_load";
  case SLAVE_EVENT:  return "Slave";
  case CREATE_FILE_EVENT: return "Create_file";
  case APPEND_BLOCK_EVENT: return "Append_block";
  case DELETE_FILE_EVENT: return "Delete_file";
  case EXEC_LOAD_EVENT: return "Exec_load";
  case RAND_EVENT: return "RAND";
  case XID_EVENT: return "Xid";
  case USER_VAR_EVENT: return "User var";
  case FORMAT_DESCRIPTION_EVENT: return "Format_desc";
  case TABLE_MAP_EVENT: return "Table_map";
  case PRE_GA_WRITE_ROWS_EVENT: return "Write_rows_event_old";
  case PRE_GA_UPDATE_ROWS_EVENT: return "Update_rows_event_old";
  case PRE_GA_DELETE_ROWS_EVENT: return "Delete_rows_event_old";
  case WRITE_ROWS_EVENT_V1: return "Write_rows_v1";
  case UPDATE_ROWS_EVENT_V1: return "Update_rows_v1";
  case DELETE_ROWS_EVENT_V1: return "Delete_rows_v1";
  case BEGIN_LOAD_QUERY_EVENT: return "Begin_load_query";
  case EXECUTE_LOAD_QUERY_EVENT: return "Execute_load_query";
  case INCIDENT_EVENT: return "Incident";
  case IGNORABLE_LOG_EVENT: return "Ignorable";
  case ROWS_QUERY_LOG_EVENT: return "Rows_query";
  case WRITE_ROWS_EVENT: return "Write_rows";
  case UPDATE_ROWS_EVENT: return "Update_rows";
  case DELETE_ROWS_EVENT: return "Delete_rows";
  case GTID_LOG_EVENT: return "Gtid";
  case ANONYMOUS_GTID_LOG_EVENT: return "Anonymous_Gtid";
  case PREVIOUS_GTIDS_LOG_EVENT: return "Previous_gtids";
  case HEARTBEAT_LOG_EVENT: return "Heartbeat";
  case USER_DEFINED: return "User defined";
  default: return "Unknown";                            /* impossible */
  }
}

} // end namespace system


Binary_log_event::~Binary_log_event()
{
}

Binary_log_event * create_incident_event(unsigned int type,
                                         const char *message, ulong pos)
{
  Incident_event *incident= new Incident_event();
  incident->header()->type_code= INCIDENT_EVENT;
  incident->header()->next_position= pos;
  incident->header()->event_length= LOG_EVENT_HEADER_SIZE + 2 + strlen(message);
  incident->type= type;
  incident->message.append(message);
  return incident;
}


void Binary_log_event::print_event_info(std::ostream& info) {}
void Binary_log_event::print_long_info(std::ostream& info) {}

void Unknown_event::print_event_info(std::ostream& info)
{
  info << "Unhandled event";
}

void Unknown_event::print_long_info(std::ostream& info)
{
  info << "Timestamp: " << this->header()->timestamp;
  this->print_event_info(info);
}

void Query_event::print_event_info(std::ostream& info)
{
  if (strcmp(query.c_str(), "BEGIN") != 0 &&
      strcmp(query.c_str(), "COMMIT") != 0)
  {
    info << "use `" << db_name << "`; ";
  }
  info << query;
}

void Query_event::print_long_info(std::ostream& info)
{
  info << "Timestamp: " << this->header()->timestamp;
  info << "\tThread id: " << (int)thread_id;
  info << "\tExec time: " << (int)exec_time;
  info << "\nDatabase: " << db_name;
  info << "\tQuery: ";
  this->print_event_info(info);
}

void Rotate_event::print_event_info(std::ostream& info)
{
  info << "Binlog Position: " << binlog_pos;
  info << ", Log name: " << binlog_file;
}

void Rotate_event::print_long_info(std::ostream& info)
{
  info << "Timestamp: " << this->header()->timestamp;
  info << "\t";
  this->print_event_info(info);
}

void Format_event::print_event_info(std::ostream& info)
{
  info << "Server ver: " << master_version;
  info << ", Binlog ver: " << binlog_version;
}

void Format_event::print_long_info(std::ostream& info)
{
  Log_event_type event_type;
  int enum_index= 1;
  this->print_event_info(info);
  info << "\nCreated timestamp: " << created_ts;
  info << "\tCommon Header Length: " << (int)log_header_len;
  info << "\nPost header length for events: \n";

  for (int i= 0; i < post_header_len.size(); i++)
  {
    event_type= static_cast<Log_event_type>(i);
    info << mysql::system::get_event_type_str(event_type)
         << "= "
         << (int)post_header_len[static_cast<Log_event_type>(i)]
         <<  "\n";
  }
}

void User_var_event::print_event_info(std::ostream& info)
{
  info << "@`" << name << "`=";
  if(type == STRING_TYPE)
    info  << value;
  else
    info << "<Binary encoded value>";
  //TODO: value is binary encoded, requires decoding
}

void User_var_event::print_long_info(std::ostream& info)
{
  info << "Timestamp: " << this->header()->timestamp;
  info << "\tType: "
       << get_value_type_string(static_cast<Value_type>(type));
  info << "\n";
  this->print_event_info(info);
}

void Table_map_event::print_event_info(std::ostream& info)
{
  info << "table id: " << table_id << " ("
       << db_name << "."
       << table_name << ")";
}

void Table_map_event::print_long_info(std::ostream& info)
{
  info << "Timestamp: " << this->header()->timestamp;
  info << "\tFlags: " << flags;
  info << "\tColumn Type: ";
  /**
    TODO: Column types are stored as integers. To be
    replaced by string representation of types.
  */
  std::vector<uint8_t>::iterator it;
  for (it= columns.begin(); it != columns.end(); ++it)
  {
    info << "\t" << (int)*it;
  }
  info << "\n";
  this->print_event_info(info);
}

void Row_event::print_event_info(std::ostream& info)
{
  info << "table id: " << table_id << " flags: ";
  info << get_flag_string(static_cast<enum_flag>(flags));
}

void Row_event::print_long_info(std::ostream& info)
{
  info << "Timestamp: " << this->header()->timestamp;
  info << "\n";
  this->print_event_info(info);

  //TODO: Extract table names and column data.
  if (this->get_event_type() == PRE_GA_WRITE_ROWS_EVENT ||
      this->get_event_type() == WRITE_ROWS_EVENT_V1 ||
      this->get_event_type() == WRITE_ROWS_EVENT)
    info << "\nType: Insert" ;

  if (this->get_event_type() == PRE_GA_DELETE_ROWS_EVENT ||
      this->get_event_type() == DELETE_ROWS_EVENT_V1 ||
      this->get_event_type() == DELETE_ROWS_EVENT)
    info << "\nType: Delete" ;

  if (this->get_event_type() == PRE_GA_UPDATE_ROWS_EVENT ||
      this->get_event_type() == UPDATE_ROWS_EVENT_V1 ||
      this->get_event_type() == UPDATE_ROWS_EVENT)
    info << "\nType: Update" ;
}

void Int_var_event::print_event_info(std::ostream& info)
{
  info << get_type_string(static_cast<Int_event_type>(type));
  info << "\tValue: " << value;
}

void Int_var_event::print_long_info(std::ostream& info)
{
  info << "Timestamp: " << this->header()->timestamp;
  info << "\t";
  this->print_event_info(info);
}

void Incident_event::print_event_info(std::ostream& info)
{
  info << message;
}

void Incident_event::print_long_info(std::ostream& info)
{
  this->print_event_info(info);
}

void Xid::print_event_info(std::ostream& info)
{
  //TODO: Write process_event function for Xid events
  info << "Xid ID=" << xid_id;
}

void Xid::print_long_info(std::ostream& info)
{
  info << "Timestamp: " << this->header()->timestamp;
  info << "\t";
  this->print_event_info(info);
}

} // end namespace mysql
