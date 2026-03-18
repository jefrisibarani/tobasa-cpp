#pragma once
#if (defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)) || defined(TOBASA_SQL_USE_ODBC)

#include <string>
#include <vector>

namespace tbs {
namespace dbm {
namespace mssql {

const std::string t_lis_headers( R"-(
   CREATE TABLE lis_headers (
      id                 bigint IDENTITY(1,1) PRIMARY KEY,
      received           datetime       NOT NULL,
      sender             varchar(24)    NULL,
      sender_1           varchar(24)    NULL,
      sender_2           varchar(24)    NULL,
      processing_id      varchar(8)     NULL,
      version            varchar(24)    NULL,
      msg_datetime       datetime       NULL,
      instrument         varchar(16)    NOT NULL,
      raw_data           nvarchar(2048) NULL,
      message_control_id varchar(50)    NULL,
      receiver_id        varchar(254)   NULL
   );
)-");

const std::string t_lis_comments( R"-(
   CREATE TABLE lis_comments (
      id          bigint IDENTITY(1,1) PRIMARY KEY,
      owner_id    bigint         NULL,
      owner_type  varchar(12)    NULL,
      seq_no      int            NOT NULL,
      data        nvarchar(256)  NULL,
      type_0      varchar(16)    NULL,
      type_1      varchar(16)    NULL,
      raw_data    nvarchar(2048) NULL
   );
)-");

const std::string t_lis_patients( R"-(
   CREATE TABLE lis_patients (
      id             bigint IDENTITY(1,1) PRIMARY KEY,
      header_id      bigint         NULL,
      seq_no         int            NOT NULL,
      practice_patid nvarchar(24)   NULL,
      lab_patid      nvarchar(24)   NULL,
      gender         varchar(8)     NULL,
      birthdate      date           NULL,
      age            varchar(24)    NULL,
      firstname      nvarchar(48)   NULL,
      middlename     nvarchar(48)   NULL,
      lastname       nvarchar(48)   NULL,
      raw_data       nvarchar(2048) NULL,
      location_id    varchar(100)   NULL
   );
)-");

const std::string t_lis_orders( R"-(
   CREATE TABLE lis_orders (
      id                   bigint IDENTITY(1,1) PRIMARY KEY,
      patient_id           bigint         NULL,
      seq_no               int            NOT NULL,
      spec_id              varchar(48)    NULL,
      spec_id1             varchar(16)    NULL,
      spec_id2             varchar(16)    NULL,
      spec_id3             varchar(16)    NULL,
      inst_spec_id         varchar(16)    NULL,
      inst_spec_id1        varchar(16)    NULL,
      test_code            varchar(32)    NULL,
      spec_coll_datetime   datetime       NULL,
      spec_type            varchar(16)    NULL,
      spec_source          varchar(16)    NULL,
      physician_name       nvarchar(48)   NULL,
      instrument           varchar(16)    NOT NULL,
      raw_data             nvarchar(2048) NOT NULL,
      test_group           varchar(50)    NULL,
      priority_id          varchar(5)     NULL,
      ordered_datetime     datetime       NULL,
      action_code          varchar(3)     NULL,
      qc_id                varchar(80)    NULL,
      qc_exp_datetime      datetime       NULL,
      qc_lot_number        varchar(80)    NULL,
      ordering_facility    varchar(80)    NULL,
      spec_dilution_factor varchar(80)    NULL,
      auto_rerun_allowed   varchar(1)     NULL,
      auto_reflex_allowed  varchar(1)     NULL,
      parent_spec_id       varchar(30)    NULL,
      result_reported      datetime       NULL,
      report_type          varchar(1)     NULL
   );
)-");

const std::string t_lis_results( R"-(
   CREATE TABLE lis_results (
      id                bigint IDENTITY(1,1) PRIMARY KEY,
      odr_id            bigint         NULL,
      odr_spec_id       varchar(48)    NULL,
      odr_inst_spec_id  varchar(16)    NULL,
      seq_no            int            NOT NULL,
      test_code         varchar(50)    NULL,
      value             varchar(512)   NULL,
      flag              varchar(8)     NULL,
      unit              varchar(50)    NULL,
      range             varchar(70)    NULL,
      abnormal_flag     varchar(8)     NULL,
      status            varchar(4)     NULL,
      operator          nvarchar(48)   NULL,
      completed         datetime       NULL,
      instrument_id     varchar(48)    NULL,
      raw_data          nvarchar(2048) NULL
   );
)-");

const std::string t_lis_requests( R"-(
   CREATE TABLE lis_requests (
      id             bigint IDENTITY(1,1) PRIMARY KEY,
      header_id      bigint         NULL,
      seq_no         int            NOT NULL,
      start_range1   varchar(254)   NULL,
      start_range2   varchar(254)   NULL,
      start_range3   varchar(254)   NULL,
      test_code      varchar(254)   NULL,
      status_code    varchar(16)    NULL,
      raw_data       nvarchar(1024) NULL
   );
)-");

const std::string t_lis_job_patients( R"-(
   CREATE TABLE lis_job_patients (
      id             bigint IDENTITY(1,1) PRIMARY KEY,
      rekmed         varchar(10)  NULL,
      practice_patid nvarchar(24) NULL,
      lab_patid      nvarchar(24) NULL,
      gender         varchar(8)   NULL,
      birthdate      date         NULL,
      age            varchar(24)  NULL,
      firstname      nvarchar(48) NULL,
      middlename     nvarchar(48) NULL,
      lastname       nvarchar(48) NULL,
      location_id    varchar(100) NULL
   );
)-");

const std::string t_lis_job_orders( R"-(
   CREATE TABLE lis_job_orders (
      id          bigint IDENTITY(1,1) PRIMARY KEY,
      order_code  varchar(30) NOT NULL,
      patient_id  bigint      NOT NULL,
      total       int         NOT NULL,
      instrument  varchar(16) NULL,
      closed      tinyint     NOT NULL DEFAULT 0 CHECK (closed IN (0, 1))
   );
)-");

const std::string t_lis_job_order_details( R"-(
   CREATE TABLE lis_job_order_details (
      id                   bigint IDENTITY(1,1) PRIMARY KEY,
      order_id             bigint         NULL,
      order_code           varchar(48)    NOT NULL,
      seq_no               int            NULL,
      spec_id              varchar(48)    NULL,
      spec_id1             varchar(16)    NULL,
      spec_id2             varchar(16)    NULL,
      spec_id3             varchar(16)    NULL,
      inst_spec_id         varchar(16)    NULL,
      inst_spec_id1        varchar(16)    NULL,
      test_code            varchar(32)    NULL,
      spec_coll_datetime   datetime       NULL,
      spec_type            varchar(16)    NULL,
      spec_source          varchar(16)    NULL,
      physician_name       nvarchar(48)   NULL,
      test_group           varchar(50)    NULL,
      priority_id          varchar(5)     NULL,
      ordered_datetime     datetime       NULL,
      action_code          varchar(3)     NULL,
      qc_id                varchar(80)    NULL,
      qc_exp_datetime      datetime       NULL,
      qc_lot_number        varchar(80)    NULL,
      ordering_facility    varchar(80)    NULL,
      spec_dilution_factor varchar(80)    NULL,
      auto_rerun_allowed   varchar(1)     NULL,
      auto_reflex_allowed  varchar(1)     NULL,
      parent_spec_id       varchar(30)    NULL,
      result_reported      datetime       NULL,
      report_type          varchar(1)     NULL,
      instrument           varchar(16)    NULL
   );
)-");

const std::string v_lis2a_tests( R"-(
   CREATE view v_lis2a_tests AS
   select h.id as h_id, p.id as p_id, o.id as o_id, r.id as r_id,
         p.seq_no as p_seq_no, o.seq_no as o_seq_no, r.seq_no as r_seq_no,
         h.received, h.msg_datetime, r.completed as r_completed, h.sender as h_sender, h.instrument as h_instrument,
         p.firstname as p_firstname, p.lastname as p_lastname,
         o.spec_id, o.test_code, o.spec_type,
         r.test_code as r_test_code, r.value as r_value, r.flag as r_flag,
         r.unit as r_unit, r.range as r_range, r.abnormal_flag as r_abnormal_flag, r.status as r_status, r.operator as r_operator
   from lis_headers h
   LEFT JOIN lis_patients p ON h.id = p.header_id
   LEFT JOIN lis_orders o ON p.id = o.patient_id
   LEFT JOIN lis_results r ON o.id = r.odr_id;
)-");

const std::string t_lis_dirui_headers( R"-(
   CREATE TABLE  lis_dirui_headers (
      id          bigint IDENTITY(1,1) PRIMARY KEY,
      received    datetime    NOT NULL,
      instrument  varchar(64) NOT NULL DEFAULT ''
   );
)-");

const std::string t_lis_dirui_tests( R"-(
   CREATE TABLE lis_dirui_tests (
      id        bigint IDENTITY(1,1) PRIMARY KEY,
      header_id bigint      NOT NULL,
      fdate     varchar(24) NOT NULL DEFAULT '',
      fno       varchar(24) NOT NULL DEFAULT '',
      fid       varchar(24) NOT NULL DEFAULT '',
      name      varchar(24) NOT NULL DEFAULT '',
      field     varchar(24) NOT NULL DEFAULT '',
      value     varchar(24) NOT NULL DEFAULT '',
      rawdata   varchar(48) NOT NULL DEFAULT ''
   );
)-");

const std::string t_lis_bci_vidas( R"-(
   CREATE TABLE lis_bci_vidas (
      id                bigint IDENTITY(1,1) PRIMARY KEY,
      received          datetime NOT NULL,
      patient_id        varchar(32) NULL,
      patient_name      varchar(40) NULL,
      patient_dob       date        NULL,
      gender            varchar(10) NULL,
      sample_origin     varchar(50) NULL,
      sample_id         varchar(50) NULL,
      short_assay       varchar(8)  NULL,
      long_assay        varchar(40) NULL,
      completed_time    time(7)     NULL,
      completed_date    date        NULL,
      qlt_result        varchar(50) NULL,
      qty_result        varchar(50) NULL,
      qn_unit           varchar(50) NULL,
      dilution          varchar(10) NULL,
      calibration_exp   varchar(50) NULL,
      instrument_id     varchar(50) NULL,
      serial_number     varchar(80) NULL,
      technologist      varchar(50) NULL
   );
)-");

const std::string t_lis_bci_vitek2compact_antibiotic( R"-(
   CREATE TABLE lis_bci_vitek2compact_antibiotic (
      id                      bigint IDENTITY(1,1) PRIMARY KEY,
      message_id              bigint         NULL,
      patient_id              varchar(64)    NULL,
      specimen_sys_code       varchar(32)    NOT NULL,
      antibiotic_family_name  varchar(64)    NULL,
      phenotype_names         varchar(1024)  NULL
   );
)-");

const std::string t_lis_bci_vitek2compact_result( R"-(
   CREATE TABLE lis_bci_vitek2compact_result (
      id                            bigint IDENTITY(1,1) PRIMARY KEY,
      message_id                    bigint      NOT NULL,
      patient_id                    varchar(64) NOT NULL,
      specimen_sys_code             varchar(32) NOT NULL,
      antibiotic_code               varchar(15) NOT NULL,
      antibiotic_name               varchar(60) NOT NULL,
      result_mic                    varchar(8)  NULL,
      final_interpretation          varchar(2)  NULL,
      non_expertised_interpretation varchar(16) NULL,
      suppressed_antibiotic_flag    varchar(1)  NULL,
      deduced_antibiotic_flag       varchar(1)  NULL,
      suppressed_mic_flag           varchar(1)  NULL
   );
)-");

const std::string t_lis_bci_vitek2compact( R"-(
   CREATE TABLE lis_bci_vitek2compact (
      id                    bigint IDENTITY(1,1) PRIMARY KEY,
      received              datetime      NOT NULL,
      instrument_code       varchar(8)    NULL,
      instrument_sn         varchar(80)   NULL,
      testgroup_code        varchar(8)    NULL,
      patient_id            varchar(64)   NOT NULL,
      patient_name          varchar(64)   NOT NULL,
      specimen_sys_code     varchar(32)   NOT NULL,
      specimen_src_code     varchar(64)   NULL,
      specimen_src_name     varchar(64)   NULL,
      specimen_collect_date date          NOT NULL,
      specimen_receipt_date date          NOT NULL,
      lab_id                varchar(24)   NOT NULL,
      lab_id_sys_code       varchar(16)   NULL,
      culture_type_code     varchar(64)   NULL,
      culture_type_name     varchar(64)   NULL,
      card_type             varchar(32)   NULL,
      isolate_sys_code      varchar(32)   NULL,
      isolate_number        varchar(8)    NULL,
      final_organism_code   varchar(64)   NULL,
      final_organism_name   varchar(64)   NULL,
      final_bionumber       varchar(64)   NULL,
      percent_probability   varchar(16)   NULL
   );
)-");

const std::string t_lis_hl7_headers( R"-(
   CREATE TABLE lis_hl7_headers (
      id                bigint IDENTITY(1,1) PRIMARY KEY,
      received          datetime       NOT NULL,
      sending_app       varchar(200)   NOT NULL DEFAULT '' ,
      sending_facility  varchar(200)   NOT NULL DEFAULT '' ,
      msg_datetime      varchar(50)    NOT NULL DEFAULT '' ,
      msg_type          varchar(7)     NOT NULL DEFAULT '' ,
      msg_control_id    varchar(50)    NOT NULL DEFAULT '' ,
      processing_id     varchar(10)    NOT NULL DEFAULT '' ,
      version_id        varchar(100)   NOT NULL DEFAULT '' ,
      app_ack_type      varchar(10)    NOT NULL DEFAULT '' ,
      character_set     varchar(50)    NOT NULL DEFAULT '' ,
      rawdata           nvarchar(2048) NOT NULL DEFAULT ''
   );
)-");

const std::string t_lis_hl7_patients( R"-(
   CREATE TABLE lis_hl7_patients (
      id             bigint IDENTITY(1,1) PRIMARY KEY,
      db_message_id  bigint      NOT NULL,
      set_id         varchar(10)    NOT NULL DEFAULT '' ,
      patient_id     varchar(30)    NOT NULL DEFAULT '' ,
      identifier     varchar(30)    NOT NULL DEFAULT '' ,
      name           nvarchar(100)  NOT NULL DEFAULT '' ,
      mother_name    nvarchar(200)  NOT NULL DEFAULT '' ,
      dob            varchar(50)    NOT NULL DEFAULT '' ,
      gender         varchar(10)    NOT NULL DEFAULT '' ,
      field_4        varchar(100)   NOT NULL DEFAULT '' ,
      field_26       varchar(100)   NOT NULL DEFAULT '' ,
      rawdata        nvarchar(2048) NOT NULL DEFAULT ''
   );
)-");

const std::string t_lis_hl7_patient_visits( R"-(
   CREATE TABLE lis_hl7_patient_visits (
      id             bigint IDENTITY(1,1) PRIMARY KEY,
      db_message_id  bigint         NOT NULL,
      db_patient_id  bigint         NOT NULL,
      room_id        varchar(50)    NOT NULL DEFAULT '' ,
      bed_id         varchar(50)    NOT NULL DEFAULT '' ,
      rawdata        varchar(2048)  NOT NULL DEFAULT ''
   );
)-");

const std::string t_lis_hl7_observation_requests( R"-(
   CREATE TABLE lis_hl7_observation_requests (
      id                   bigint IDENTITY(1,1) PRIMARY KEY,
      db_message_id        bigint         NOT NULL,
      db_patient_id        bigint         NOT NULL,
      set_id               varchar(10)    NOT NULL DEFAULT '' ,
      identifier           varchar(50)    NOT NULL DEFAULT '' ,
      universal_id         varchar(200)   NOT NULL DEFAULT '' ,
      universal_name       varchar(200)   NOT NULL DEFAULT '' ,
      universal_encodesys  varchar(200)   NOT NULL DEFAULT '' ,
      priority_obr         varchar(4)     NOT NULL DEFAULT '' ,
      requested_datetime   varchar(50)    NOT NULL DEFAULT '' ,
      observation_datetime varchar(50)    NOT NULL DEFAULT '' ,
      veterinarian         nvarchar(60)   NOT NULL DEFAULT '' ,
      clinical_info        varchar(512)   NOT NULL DEFAULT '' ,
      speciment_datetime   varchar(50)    NOT NULL DEFAULT '' ,
      sending_doctor       nvarchar(200)  NOT NULL DEFAULT '' ,
      sending_department   varchar(200)   NOT NULL DEFAULT '' ,
      placer_field1        varchar(60)    NOT NULL DEFAULT '' ,
      diag_maker_id        varchar(10)    NOT NULL DEFAULT '' ,
      parent_result        varchar(254)   NOT NULL DEFAULT '' ,
      result_copies_to     varchar(100)   NOT NULL DEFAULT '' ,
      principal_result     varchar(254)   NOT NULL DEFAULT '' ,
      rawdata              nvarchar(2048) NOT NULL DEFAULT ''
   );
)-");

const std::string t_lis_hl7_observation_results( R"-(
   CREATE TABLE lis_hl7_observation_results (
      id                    bigint IDENTITY(1,1) PRIMARY KEY,
      db_message_id         bigint        NOT NULL,
      db_patient_id         bigint        NOT NULL,
      db_obr_id             bigint        NOT NULL,
      set_id                varchar(10)   NOT NULL DEFAULT '',
      value_type            varchar(5)    NOT NULL DEFAULT '' ,
      identifier_id         varchar(1024) NOT NULL DEFAULT '' ,
      identifier_name       varchar(1024) NOT NULL DEFAULT '' ,
      identifier_encode_sys varchar(1024) NOT NULL DEFAULT '' ,
      result_value          varchar(2048) NOT NULL DEFAULT '' ,
      result_unit           varchar(100)  NOT NULL DEFAULT '' ,
      references_range      varchar(100)  NOT NULL DEFAULT '' ,
      abnormal_flags        varchar(10)   NOT NULL DEFAULT '' ,
      result_status         varchar(10)   NOT NULL DEFAULT '' ,
      user_defined_checks   varchar(30)   NOT NULL DEFAULT '' ,
      observation_datetime  varchar(50)   NOT NULL DEFAULT '' ,
      responsible_observer  varchar(200)  NOT NULL DEFAULT '' ,
      binary_value          tinyint       NOT NULL DEFAULT 0 CHECK (binary_value IN (0, 1)),
      binary_app            varchar(100)  NOT NULL DEFAULT '' ,
      binary_type           varchar(100)  NOT NULL DEFAULT '' ,
      binary_encoding       varchar(100)  NOT NULL DEFAULT '' ,
      binary_data           varchar(max)  NOT NULL DEFAULT '' ,
      rawdata               varchar(max)  NOT NULL DEFAULT ''
   );
)-");

const std::string t_lis_message_http_upload( R"-(
   CREATE TABLE lis_message_http_upload (
      id                bigint IDENTITY(1,1) PRIMARY KEY,
      instrument_type   varchar(50)  NOT NULL DEFAULT '',
      header_id         bigint       NOT NULL DEFAULT -1,
      start_time        datetime     NOT NULL,
      response_time     datetime     NULL,
      status            varchar(100) NOT NULL DEFAULT ''
   );
)-");

const std::string t_lis_combostik_headers( R"-(
   CREATE TABLE lis_combostik_headers (
      id             bigint IDENTITY(1,1) PRIMARY KEY,
      received       datetime    NOT NULL,
      msg_datetime   datetime    NULL,
      msg_id         varchar(26) NOT NULL DEFAULT '' ,
      msg_sn         varchar(26) NOT NULL DEFAULT '' ,
      msg_op         varchar(26) NOT NULL DEFAULT '' ,
      msg_lot        varchar(26) NOT NULL DEFAULT '' ,
      strip_no       varchar(26) NOT NULL DEFAULT '' ,
      pat_id         varchar(26) NOT NULL DEFAULT '' ,
      pat_name       varchar(26) NOT NULL DEFAULT '' ,
      pat_gender     int         NOT NULL DEFAULT -1 ,
      pat_age        varchar(26) NOT NULL DEFAULT '' ,
      ward_name      varchar(26) NOT NULL DEFAULT '' ,
      instrument     varchar(64) NOT NULL DEFAULT ''
   );
)-");

const std::string t_lis_combostik_tests( R"-(
   CREATE TABLE lis_combostik_tests(
      id          bigint IDENTITY(1,1) PRIMARY KEY,
      header_id   bigint      NOT NULL,
      msg_id      varchar(26) NOT NULL DEFAULT '' ,
      pos         int         NOT NULL DEFAULT -1 ,
      name        varchar(26) NOT NULL DEFAULT '' ,
      field       varchar(26) NOT NULL DEFAULT '' ,
      value       varchar(26) NOT NULL DEFAULT '' ,
      note        varchar(26) NOT NULL DEFAULT ''
   );
)-");

inline std::vector<std::string> getLisQueries()
{
   std::vector<std::string> queries;

   queries.push_back(t_lis_headers);
   queries.push_back(t_lis_comments);
   queries.push_back(t_lis_patients);
   queries.push_back(t_lis_orders);
   queries.push_back(t_lis_results);
   queries.push_back(t_lis_requests);
   queries.push_back(t_lis_job_patients);
   queries.push_back(t_lis_job_orders);
   queries.push_back(t_lis_job_order_details);
   queries.push_back(v_lis2a_tests);
   
   queries.push_back(t_lis_dirui_headers);
   queries.push_back(t_lis_dirui_tests);
   queries.push_back(t_lis_bci_vidas);
   queries.push_back(t_lis_bci_vitek2compact_antibiotic);
   queries.push_back(t_lis_bci_vitek2compact_result);
   queries.push_back(t_lis_bci_vitek2compact);

   queries.push_back(t_lis_hl7_headers);
   queries.push_back(t_lis_hl7_patients);
   queries.push_back(t_lis_hl7_patient_visits);
   queries.push_back(t_lis_hl7_observation_requests);
   queries.push_back(t_lis_hl7_observation_results);

   queries.push_back(t_lis_message_http_upload);

   queries.push_back(t_lis_combostik_headers);
   queries.push_back(t_lis_combostik_tests);

   return queries;
}

} // namesapce mssql
} // namespace dbm
} // namespace tbs

#endif // (defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)) || defined(TOBASA_SQL_USE_ODBC)