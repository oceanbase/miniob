/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
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
#include "common/seda/seda_config.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <typeinfo>
#include <vector>

#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/os/os.h"
#include "common/seda/init.h"
#include "common/seda/stage_factory.h"
namespace common {

SedaConfig *SedaConfig::instance_ = NULL;



SedaConfig *&SedaConfig::get_instance() {
  if (instance_ == NULL) {
    instance_ = new SedaConfig();
    ASSERT((instance_ != NULL), "failed to allocate SedaConfig");
  }
  return instance_;
}

// Constructor
SedaConfig::SedaConfig() : cfg_file_(), cfg_str_(), thread_pools_(), stages_() {
  return;
}

// Destructor
SedaConfig::~SedaConfig() {
  ASSERT(instance_, "Instance should not be null");
  // check to see if clean-up is necessary
  if ((!thread_pools_.empty()) || (!stages_.empty())) {
    cleanup();
  }

  instance_ = NULL;
}

// Set the file holding the configuration
void SedaConfig::set_cfg_filename(const char *filename) {
  cfg_str_.clear();
  cfg_file_.clear();
  if (filename != NULL) {
    cfg_file_.assign(filename);
  }
  return;
}

// Set the string holding the configuration
void SedaConfig::set_cfg_string(const char *config_str) {
  cfg_str_.clear();
  cfg_file_.clear();
  if (config_str != NULL) {
    cfg_str_.assign(config_str);
  }
  return;
}

// Parse config file or string
SedaConfig::status_t SedaConfig::parse() {
  // first parse the config
  try {
    // skip parse in this implementation
    // all configuration will be put into one file
  } catch (const std::exception &e) {
    std::string err_msg(e.what());
    LOG_ERROR("Seda config parse failed w/ error %s", err_msg.c_str());
    return PARSEFAIL;
  }
  LOG_DEBUG("Seda config parse success\n");

  return SUCCESS;
}

// instantiate the parsed SEDA configuration
SedaConfig::status_t SedaConfig::instantiate_cfg() {
  status_t stat = SUCCESS;

  // instantiate the configuration
  stat = instantiate();

  return stat;
}

// start the configuration - puts the stages_ into action
SedaConfig::status_t SedaConfig::start() {
  status_t stat = SUCCESS;

  ASSERT(thread_pools_.size(), "Configuration not yet instantiated");

  // start the stages_ one by one.  connect() calls
  std::map<std::string, Stage *>::iterator iter = stages_.begin();
  std::map<std::string, Stage *>::iterator end = stages_.end();

  while (iter != end) {
    if (iter->second != NULL) {
      Stage *stg = iter->second;
      bool ret = stg->connect();
      if (!ret) {
        cleanup();
        stat = INITFAIL;
        break;
      }
    }
    iter++;
  }

  return stat;
}

// Initialize the thread_pools_ and stages_
SedaConfig::status_t SedaConfig::init() {
  status_t stat = SUCCESS;

  // check the preconditions
  ASSERT(stages_.empty(), "Attempt to initialize sedaconfig twice");
  ASSERT(thread_pools_.empty(), "Attempt to initialize sedaconfig twice");

  // instantiate the parsed config
  stat = instantiate();
  if (stat) {
    return stat;
  }

  // start it running
  stat = start();
  if (stat) {
    return stat;
  }

  return SUCCESS;
}

// Clean-up the threadpool and stages_
void SedaConfig::cleanup() {
  // first disconnect all stages_
  if (stages_.empty() == false) {
    std::map<std::string, Stage *>::iterator iter = stages_.begin();
    std::map<std::string, Stage *>::iterator end = stages_.end();
    while (iter != end) {
      if (iter->second != NULL) {
        Stage *stg = iter->second;
        if (stg->is_connected()) {
          stg->disconnect();
        }
      }
      iter++;
    }
  }
  LOG_TRACE("stages_ disconnected\n");

  // now delete all stages_ and thread_pools_
  clear_config();
}

void SedaConfig::init_event_history() {
  std::map<std::string, std::string> base_section =
    get_properties()->get(SEDA_BASE_NAME);
  std::map<std::string, std::string>::iterator it;
  std::string key;

  // check whether event histories are enabled
  bool ev_hist = false;
  key = EVENT_HISTORY;
  it = base_section.find(key);
  if (it != base_section.end()) {
    if (it->second.compare("true") == 0) {
      ev_hist = true;
    }
  }

  get_event_history_flag() = ev_hist;

  // set max event hops
  u32_t max_event_hops = 100;
  key = MAX_EVENT_HISTORY_NUM;
  it = base_section.find(key);
  if (it != base_section.end()) {
    str_to_val(it->second, max_event_hops);
  }
  get_max_event_hops() = max_event_hops;

  LOG_INFO("Successfully init_event_history, EventHistory:%d, MaxEventHops:%u",
           (int) ev_hist, max_event_hops);
  return;
}

SedaConfig::status_t SedaConfig::init_thread_pool() {
  try {

    std::map<std::string, std::string> base_section =
      get_properties()->get(SEDA_BASE_NAME);
    std::map<std::string, std::string>::iterator it;
    std::string key;

    // get thread pool names
    key = THREAD_POOLS_NAME;
    it = base_section.find(key);
    if (it == base_section.end()) {
      LOG_ERROR("Configuration hasn't set %s", key.c_str());
      return INITFAIL;
    }

    std::string pool_names = it->second;
    std::vector<std::string> name_list;
    std::string split_tag;
    split_tag.assign(1, Ini::CFG_DELIMIT_TAG);
    split_string(pool_names, split_tag, name_list);

    int cpu_num = getCpuNum();
    std::string default_cpu_num_str;
    val_to_str(cpu_num, default_cpu_num_str);

    for (size_t pos = 0; pos != name_list.size(); pos++) {
      std::string &thread_name = name_list[pos];

      // get count number
      key = COUNT;
      std::string count_str = get_properties()->get(key, default_cpu_num_str, thread_name);

      int thread_count = 1;
      str_to_val(count_str, thread_count);
      if (thread_count < 1) {
        LOG_INFO("Thread number of  %s is %d, it is same as cpu's cores.",
                  thread_name.c_str(), cpu_num);
        thread_count = cpu_num;
      }
      const int max_thread_count = 1000000;
      if (thread_count >= max_thread_count) {
        LOG_ERROR("Thread number is too big: %d(max:%d)", thread_count, max_thread_count);
        return INITFAIL;
      }

      Threadpool * thread_pool = new Threadpool(thread_count, thread_name);
      if (thread_pool == NULL) {
        LOG_ERROR("Failed to new %s threadpool\n", thread_name.c_str());
        return INITFAIL;
      }
      thread_pools_[thread_name] = thread_pool;
    }

    if  (thread_pools_.find(DEFAULT_THREAD_POOL) == thread_pools_.end()) {
      LOG_ERROR("There is no default thread pool %s, please add it.",
                DEFAULT_THREAD_POOL);
      return INITFAIL;
    }

  } catch (std::exception &e) {
    LOG_ERROR("Failed to init thread_pools_:%s\n", e.what());
    clear_config();
    return INITFAIL;
  }

  int pools = thread_pools_.size();
  if (pools < 1) {
    LOG_ERROR("Invalid number of thread_pools_:%d", pools);
    clear_config();
    return INITFAIL;
  }

  return SUCCESS;
}

std::string SedaConfig::get_thread_pool(std::string &stage_name) {
  std::string ret = DEFAULT_THREAD_POOL;
  // Get thread pool
  std::map<std::string, std::string> stage_section =
      get_properties()->get(stage_name);
  std::map<std::string, std::string>::iterator itt;
  std::string thread_pool_id = THREAD_POOL_ID;
  itt = stage_section.find(thread_pool_id);
  if (itt == stage_section.end()) {
    LOG_INFO("Not set thread_pool_id for %s, use default threadpool %s",
             stage_name.c_str(), DEFAULT_THREAD_POOL);
    return ret;
  }

  std::string thread_name = itt->second;
  if (thread_name.empty()) {
    LOG_ERROR("Failed to set %s of the %s, use the default threadpool %s",
              thread_pool_id.c_str(), stage_name.c_str(), DEFAULT_THREAD_POOL);
    return ret;
  }

  if (thread_pools_.find(thread_name) == thread_pools_.end()) {
    LOG_ERROR("The stage %s's threadpool %s is invalid, use the default "
              "threadpool %s",
              stage_name.c_str(), thread_name.c_str(), DEFAULT_THREAD_POOL);
  }
  return ret;
}

SedaConfig::status_t SedaConfig::init_stages() {
  try {
    std::map<std::string, std::string> base_section =
      get_properties()->get(SEDA_BASE_NAME);
    std::map<std::string, std::string>::iterator it;
    std::string key;

    // get stage names
    key = STAGES;
    it = base_section.find(key);
    if (it == base_section.end()) {
      LOG_ERROR("Hasn't set stages name in %s", key.c_str());
      clear_config();
      return INITFAIL;
    }

    std::string split_tag;
    split_tag.assign(1, Ini::CFG_DELIMIT_TAG);
    split_string(it->second, split_tag, stage_names_);

    for (std::vector<std::string>::iterator it = stage_names_.begin();
         it != stage_names_.end(); it++) {
      std::string stage_name(*it);

      std::string thread_name = get_thread_pool(stage_name);
      Threadpool *t = thread_pools_[thread_name];

      Stage *stage = StageFactory::make_instance(stage_name);
      if (stage == NULL) {
        LOG_ERROR("Failed to make instance of stage %s", stage_name.c_str());
        clear_config();
        return INITFAIL;
      }
      stages_[stage_name] = stage;
      stage->set_pool(t);

      LOG_INFO("Stage %s use threadpool %s.",
               stage_name.c_str(), thread_name.c_str());
    } // end for stage

  } catch (std::exception &e) {
    LOG_ERROR("Failed to parse stages information, please check, err:%s",
              e.what());
    clear_config();
    return INITFAIL;
  }

  if (stages_.size() < 1) {
    LOG_ERROR("Invalid number of stages_: %lu\n", stages_.size());
    clear_config();
    return INITFAIL;
  }

  return SUCCESS;
}

SedaConfig::status_t SedaConfig::gen_next_stages() {
  try {
    for (std::vector<std::string>::iterator it_name = stage_names_.begin();
         it_name != stage_names_.end(); it_name++) {

      std::string stage_name(*it_name);
      Stage *stage = stages_[stage_name];

      std::map<std::string, std::string> stage_section =
        get_properties()->get(stage_name);
      std::map<std::string, std::string>::iterator it;
      std::string next_stage_id = NEXT_STAGES;
      it = stage_section.find(next_stage_id);
      if (it == stage_section.end()) {
        continue;
      }

      std::string next_stage_names = it->second;

      std::vector<std::string> next_stage_name_list;
      std::string split_tag;
      split_tag.assign(1, Ini::CFG_DELIMIT_TAG);
      split_string(next_stage_names, split_tag, next_stage_name_list);

      for (std::vector<std::string>::iterator next_it =
        next_stage_name_list.begin();
           next_it != next_stage_name_list.end(); next_it++) {
        std::string &next_stage_name = *next_it;
        Stage *next_stage = stages_[next_stage_name];
        stage->push_stage(next_stage);
      }

    } // end for stage
  } catch (std::exception &e) {
    LOG_ERROR("Failed to get next stages");
    clear_config();
    return INITFAIL;
  }
  return SUCCESS;
}

// instantiate the thread_pools_ and stages_
SedaConfig::status_t SedaConfig::instantiate() {

  init_event_history();

  SedaConfig::status_t status = init_thread_pool();
  if (status) {
    LOG_ERROR("Failed to init thread pool\n");
    return status;
  }

  status = init_stages();
  if (status) {
    LOG_ERROR("Failed init stages_\n");
    return status;
  }

  status = gen_next_stages();
  if (status) {
    LOG_ERROR("Failed to generate next stage list\n");
    return status;
  }

  return SUCCESS;
}

// delete all thread_pools_ and stages_
void SedaConfig::clear_config() {
  // delete stages_
  std::map<std::string, Stage *>::iterator s_iter = stages_.begin();
  std::map<std::string, Stage *>::iterator s_end = stages_.end();
  while (s_iter != s_end) {
    if (s_iter->second != NULL) {

      Stage *stg = s_iter->second;
      LOG_INFO("Stage %s deleted.", stg->get_name());
      ASSERT((!stg->is_connected()), "%s%s", "Stage connected in clear_config ",
             stg->get_name());
      delete stg;
      s_iter->second = NULL;
    }
    s_iter++;
  }
  stages_.clear();
  LOG_INFO("Seda Stage released");

  // delete thread_pools_
  std::map<std::string, Threadpool *>::iterator t_iter = thread_pools_.begin();
  std::map<std::string, Threadpool *>::iterator t_end = thread_pools_.end();
  while (t_iter != t_end) {
    if (t_iter->second != NULL) {
      LOG_INFO("ThreadPool %s deleted.", t_iter->first.c_str());
      delete t_iter->second;
      t_iter->second = NULL;
    }
    t_iter++;
  }
  thread_pools_.clear();
  LOG_INFO("Seda thread pools released");
}

void SedaConfig::get_stage_names(std::vector<std::string> &names) const {
  names = stage_names_;
}

void SedaConfig::get_stage_queue_status(std::vector<int> &stats) const {
  for (std::map<std::string, Stage *>::const_iterator i = stages_.begin();
       i != stages_.end(); ++i) {
    Stage *stg = (*i).second;
    stats.push_back(stg->qlen());
  }
}

// Global seda config object
SedaConfig *&get_seda_config() {
  static SedaConfig *seda_config = NULL;

  return seda_config;
}

} //namespace common