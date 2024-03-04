#!/usr/bin/env sysbench
-- Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
-- miniob is licensed under Mulan PSL v2.
-- You can use this software according to the terms and conditions of the Mulan PSL v2.
-- You may obtain a copy of Mulan PSL v2 at:
--         http://license.coscl.org.cn/MulanPSL2
-- THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
-- EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
-- MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
-- See the Mulan PSL v2 for more details.


require("miniob_common")

sysbench.cmdline.commands.prepare = {
    function ()
        cmd_prepare()
    end,
    sysbench.cmdline.PARALLEL_COMMAND
}

function prepare_statements()

end

function event()
    local table_name = "sbtest" .. sysbench.rand.uniform(1, sysbench.opt.tables)
    local k_val = sysbench.rand.default(1, sysbench.opt.table_size)

    local query = "SELECT * FROM %s WHERE k=%d"
    con:query(string.format(query, table_name, k_val))

    check_reconnect()

end
