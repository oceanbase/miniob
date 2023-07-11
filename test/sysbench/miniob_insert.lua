#!/usr/bin/env sysbench
-- Copyright (c) 2023 OceanBase and/or its affiliates. All rights reserved.
-- miniob is licensed under Mulan PSL v2.
-- You can use this software according to the terms and conditions of the Mulan PSL v2.
-- You may obtain a copy of Mulan PSL v2 at:
--         http://license.coscl.org.cn/MulanPSL2
-- THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
-- EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
-- MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
-- See the Mulan PSL v2 for more details.

-- ----------------------------------------------------------------------
-- Insert-Only OLTP benchmark
-- ----------------------------------------------------------------------

require("miniob_common")

sysbench.cmdline.commands.prepare = {
   function ()
      cmd_prepare()
   end,
   sysbench.cmdline.PARALLEL_COMMAND
}

function prepare_statements()
   -- We do not use prepared statements here, but oltp_common.sh expects this
   -- function to be defined
end

function event()
   local table_name = "sbtest" .. sysbench.rand.uniform(1, sysbench.opt.tables)
   local k_val = sysbench.rand.default(1, sysbench.opt.table_size)
   local c_val = get_c_value()
   local pad_val = get_pad_value()
   local f_val = get_f_value()

   con:query(string.format("INSERT INTO %s VALUES " ..
                              "(%d, %f, '%s', '%s')",
                           table_name, k_val, f_val, c_val, pad_val))


   check_reconnect()
end

