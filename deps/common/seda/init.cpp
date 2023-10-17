/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Longda on 2010
//

#include "common/seda/init.h"

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <string>

#include "common/io/io.h"
#include "common/log/log.h"
#include "common/time/datetime.h"
#include "common/seda/kill_thread.h"
#include "common/seda/seda_config.h"
#include "common/seda/stage_factory.h"
#include "common/seda/metrics_stage.h"
#include "common/seda/thread_pool.h"
#include "common/seda/timer_stage.h"
namespace common {

int init_seda(ProcessParam *process_cfg)
{
  // Initialize the static data structures of threadpool
  Threadpool::create_pool_key();

  // initialize class factory instances here
  static StageFactory kill_thread_factory("KillThreads", &KillThreadStage::make_stage);
  static StageFactory timer_factory("TimerStage", &TimerStage::make_stage);
  static StageFactory seda_stats_factory("MetricsStage", &MetricsStage::make_stage);

  // try to parse the seda configuration files
  SedaConfig *config = SedaConfig::get_instance();
  SedaConfig::status_t config_stat;

  config_stat = config->parse();
  if (config_stat != SedaConfig::SUCCESS) {
    LOG_ERROR("Error: unable to parse file %s", process_cfg->get_process_name().c_str());
    return errno;
  }

  // Log a message to indicate that we are restarting, when looking
  // at a log we can see if mmon is restarting us because we keep
  // crashing.
  LOG_INFO("(Re)Starting State: Pid: %u Time: %s", (unsigned int)getpid(), DateTime::now().to_string_local().c_str());
  LOG_INFO("The process Name is %s", process_cfg->get_process_name().c_str());

  // try to initialize the seda configuration
  config_stat = config->init();
  if (config_stat != SedaConfig::SUCCESS) {
    LOG_ERROR("SedaConfig: unable to initialize seda stages");
    return errno;
  }

  get_seda_config() = config;

  return 0;
}

void cleanup_seda()
{
  SedaConfig *seda_config = SedaConfig::get_instance();
  delete seda_config;
  SedaConfig::get_instance() = NULL;
}

}  // namespace common