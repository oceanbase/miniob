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

#ifndef __COMMON_SEDA_SEDA_CONFIG_H__
#define __COMMON_SEDA_SEDA_CONFIG_H__

// Include Files
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common/seda/thread_pool.h"
#include "common/seda/seda_defs.h"

namespace common {

// keywords of sedaconfig

/**
 *  A class to configure seda stages
 *  Each application uses an xml file to define the stages that make up the
 *  application, the threadpool that the stages use, and the parameters that
 *  are passed to configure each individual stage.  The SedaConfig class
 *  consumes this xml file, parses it, and instantiates the indicated
 *  configuration.  It also then provides access to individual stages_ and
 *  threadpools within the configuration. The parameters passed to each
 *  stage consists of the global attributes defined for all the seda stages_
 *  in the seda instance as well as the attributes defined for that specific
 *  stage. The attributes defined for each stage will override the global
 *  attributes in case of duplicate attributes
 */

class SedaConfig {

public:
  typedef enum { SUCCESS = 0, INITFAIL, PARSEFAIL } status_t;

  static SedaConfig *&get_instance();

  /**
   * Destructor
   * @post configuration is deleted
   */
  ~SedaConfig();

  /**
   * Set the file holding the configuration
   * @pre  filename is a null-terminated string, or \c NULL
   * @post config filename is initialized, config string is empty
   */
  void set_cfg_filename(const char *filename);

  /**
   * Set the string holding the configuration
   * @pre  config_str is a null-terminated string, or \c NULL
   * @post config string is initialized, config filename is empty
   */
  void set_cfg_string(const char *config_str);

  /**
   * Parse config file or string
   * parse the seda config file or string and build an in-memory
   * representation of the config.  Also, update global properties object
   * with global seda properties from the config.
   *
   * @post config file or string is parsed and global seda properties
   *       are added to global properties object
   * @returns SUCCESS if parsing succeeds
   *          PARSEFAIL if parsing fails
   */
  status_t parse();

  /**
   * instantiate the parsed configuration
   * Use the parsed configuration to instantiate the thread pools
   * and stages_ specificed, but do not start it running.
   *
   * @pre  configuration has been successfully parsed
   * @post upon SUCCESS, thread pools and stages_ are created,
   *        ready to be started.
   */
  status_t instantiate_cfg();

  /**
   * start the parsed, instantiated configuration
   * @pre   configuration parsed and instantiated
   * @post  if SUCCESS, the SEDA pipleine is now running.
   *
   */
  status_t start();

  /**
   * Complete Initialization of the mThreadPools and stages_
   * Use the parsed config to initialize the required mThreadPools and
   * stages_, and start them running.  If the config has not yet been
   * parsed then try to parse it first.  The init function combines
   * parse(), instantiate() and start()
   *
   * @pre empty mThreadPools and stages_
   * @post if returns SUCCESS then
   *          mThreadPools and stages_ created/initialized and running
   * @post if returns INITFAIL or PARSEFAIL then
   *          mThreadPools and stage list are empty
   */
  status_t init();

  /**
   * Clean-up the threadpool and stages_
   * @post all stages_ disconnected and deleted, all mThreadPools deleted
   */
  void cleanup();

  /**
   * get the desired stage given a string
   *
   * @param[in] stagename   take in the stage name and convert it to a Stage
   * @pre
   * @return a reference to the Stage
   */
  Stage *get_stage(const char *stagename);

  /**
   * get the desired threadpool a string
   *
   * @param[in] index   take in the index for threadpool
   * @pre
   * @return a reference to the ThreadPool
   */
  Threadpool &get_thread_pool(const int index);

  /**
   * Get a list of all stage names
   * @param[in/out] names   names of all stages_
   */
  void get_stage_names(std::vector<std::string> &names) const;

  /**
   * Query the number of queued events at each stage.
   * @param[in/out] stats   number of events enqueued at each
   *   stage.
   */
  void get_stage_queue_status(std::vector<int> &stats) const;

  std::map<std::string, Stage *>::iterator begin();
  std::map<std::string, Stage *>::iterator end();

private:
  // Constructor
  SedaConfig();

  /**
   * instantiate the mThreadPools and stages_
   * Instantiate the mThreadPools and stages_ defined in the configuration
   *
   * @pre  cfg_ptr is not NULL
   * @post returns SUCCESS ==> all mThreadPools and stages_ are created
   *       returns INITFAIL ==> mThreadPools and stages_ are deleted
   */
  status_t instantiate();

  status_t init_thread_pool();

  std::string get_thread_pool(std::string &stage_name);

  status_t init_stages();
  status_t gen_next_stages();

  /**
   * delete all mThreadPools and stages_
   * @pre  all existing stages_ are disconnected
   * @post all mThreadPools and stages_ are deleted
   */
  void clear_config();

  /**
   * init event history setting
   * Setting max_event_hops, event_history_flag
   */
  void init_event_history();

  SedaConfig &operator=(const SedaConfig &cevtout);

  static SedaConfig *instance_;

  // In old logic, SedaConfig will parse seda configure file
  // but here, only one configure file
  std::string cfg_file_;
  std::string cfg_str_;

  std::map<std::string, Threadpool *> thread_pools_;
  std::map<std::string, Stage *> stages_;
  std::vector<std::string> stage_names_;
};

inline std::map<std::string, Stage *>::iterator SedaConfig::begin()
{
  return stages_.begin();
}

inline std::map<std::string, Stage *>::iterator SedaConfig::end()
{
  return stages_.end();
}

inline Stage *SedaConfig::get_stage(const char *stagename)
{
  if (stagename) {
    std::string sname(stagename);
    return stages_[stagename];
  }
  return NULL;
}

// Global seda config object
SedaConfig *&get_seda_config();

bool &get_event_history_flag();
u32_t &get_max_event_hops();

}  // namespace common
#endif  //__COMMON_SEDA_SEDA_CONFIG_H__
