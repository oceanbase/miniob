#!/usr/bin/env sysbench
-- Copyright (C) 2006-2017 Alexey Kopytov <akopytov@gmail.com>

-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.

-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.

-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

-- ----------------------------------------------------------------------
-- Insert-Only OLTP benchmark
-- ----------------------------------------------------------------------

require("miniob_common")

sysbench.cmdline.commands.prepare = {
    function()
        cmd_prepare()
    end,
    sysbench.cmdline.PARALLEL_COMMAND
}

function prepare_statements()

end

function event()
    local table_name = "sbtest" .. sysbench.rand.uniform(1, sysbench.opt.tables)
        local k = sysbench.rand.default(1, sysbench.opt.table_size)

    con:query(string.format("DELETE FROM %s WHERE k=%d", table_name, k))

    check_reconnect()
end