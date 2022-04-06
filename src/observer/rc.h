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
// Created by Longda on 2021/5/2.
//

#ifndef __OBSERVER_RC_H__
#define __OBSERVER_RC_H__

#define rc2SimpleStr(rc) #rc

enum RCBufferPool {
  BP_EXIST = 1,
  BP_FILEERR,
  BP_INVALIDNAME,
  BP_WINDOWS,
  BP_CLOSED,
  BP_OPEN,
  BP_NOBUF,
  BP_EOF,
  BP_INVALIDPAGENUM,
  BP_NOTINBUF,
  BP_PAGE_PINNED,
  BP_OPEN_TOO_MANY_FILES,
  BP_ILLEGAL_FILE_ID,
};

enum RCRecord {
  RD_CLOSED = 1,
  RD_OPENNED,
  RD_INVALIDRECSIZE,
  RD_INVALIDRID,
  RD_NO_MORE_RECINMEM,
  RD_OPEN,
  RD_NO_MORE_IDXINMEM,
  RD_INVALID_KEY,
  RD_DUPLICATE_KEY,
  RD_NOMEM,
  RD_SCANCLOSED,
  RD_SCANOPENNED,
  RD_EOF,
  RD_NOT_EXIST,
};

enum RCSchema {
  DB_EXIST = 1,
  DB_NOT_EXIST,
  DB_NOT_OPENED,
  TABLE_NOT_EXIST,
  TABLE_EXIST,
  TABLE_NAME_ILLEGAL,
  FIELD_NOT_EXIST,
  FIELD_EXIST,
  FIELD_NAME_ILLEGAL,
  FIELD_MISSING,
  FIELD_REDUNDAN,
  FIELD_TYPE_MISMATCH,
  INDEX_NAME_REPEAT,
  INDEX_EXIST,
  INDEX_NOT_EXIST,
  INDEX_NAME_ILLEGAL,
};

enum RCSQL { SQL_SELECT = 1 };

enum RCIOError {
  READ = 1,
  SHORT_READ,
  WRITE,
  FSYNC,
  DIR_FSYNC,
  TRUNCATE,
  FSTAT,
  IO_DELETE,
  BLOCKED,
  ACCESS,
  CHECKRESERVEDLOCK,
  CLOSE,
  DIR_CLOSE,
  SHMOPEN,
  SHMSIZE,
  SHMLOCK,
  SHMMAP,
  SEEK,
  DELETE_NOENT,
  MMAP,
  GETTEMPPATH,
  IO_CONVPATH,
  VNODE,
  BEGIN_ATOMIC,
  COMMIT_ATOMIC,
  ROLLBACK_ATOMIC,
  DATA,
  CORRUPTFS,
  OPEN_TOO_MANY_FILES,
};

enum RCLock {
  LOCK = 1,
  UNLOCK,
  SHAREDCACHE,
  LVIRT,
  NEED_WAIT,
  RESOURCE_DELETED,
};

enum RCBusy {
  BRECOVERY = 1,
  SNAPSHOT,
  TIMEOUT,
};

enum RCCantOpen {
  NOTEMPDIR = 1,
  ISDIR,
  FULLPATH,
  CO_CONVPATH,
  DIRTYWAL,
  SYMLINK,
};

enum RCCorrupt { CORRUPT_VIRT = 1, CORRUPT_SEQUENCE, CORRUPT_INDEX };

enum RCReadonly {
  RO_RECOVERY = 1,
  CANTLOCK,
  RO_ROLLBACK,
  DBMOVED,
  CANTINIT,
  DIRECTORY,
};

enum RCAbort {
  AROLLBACK = 1,
};

enum RCContraint {
  CHECK = 1,
  COMMITHOOK,
  FOREIGNKEY,
  FUNCTION,
  NOTNULL,
  PRIMARYKEY,
  TRIGGER,
  UNIQUE,
  CVIRT,
  ROWID,
  PINNED,
};

enum RCNotice {
  RECOVER_WAL = 1,
  RECOVER_ROLLBACK,
  AUTOINDEX,
};

enum RCAuth {
  USER = 1,
};

enum RC {

  SUCCESS = 0, /* Successful result */
  /* beginning-of-error-codes */
  GENERIC_ERROR,    /* Generic error */
  INVALID_ARGUMENT, /* Invalid argument */
  SQL_SYNTAX,       /* SQL Syntax error */
  BUFFERPOOL,       /* Buffer pool error*/
  RECORD,           /* Record error */
  INTERNAL,         /* Internal logic error in SQLite */
  PERM,             /* Access permission denied */
  ABORT,            /* Callback routine requested an abort */
  BUSY,             /* The database file is locked */
  LOCKED,           /* A table in the database is locked */
  NOMEM,            /* A malloc() failed */
  READONLY,         /* Attempt to write a readonly database */
  INTERRUPT,        /* Operation terminated by interrupt()*/
  IOERR,            /* Some kind of disk I/O error occurred */
  CORRUPT,          /* The database disk image is malformed */
  NOTFOUND,         /* Unknown opcode in file_control() */
  FULL,             /* Insertion failed because database is full */
  CANTOPEN,         /* Unable to open the database file */
  PROTOCOL,         /* Database lock protocol error */
  EMPTY,            /* Internal use only */
  SCHEMA,           /* The database schema error */
  TOOBIG,           /* String or BLOB exceeds size limit */
  CONSTRAINT,       /* Abort due to constraint violation */
  MISMATCH,         /* Data type mismatch */
  MISUSE,           /* Library used incorrectly */
  NOLFS,            /* Uses OS features not supported on host */
  AUTH,             /* Authorization denied */
  FORMAT,           /* Not used */
  RANGE,            /* 2nd parameter to bind out of range */
  NOTADB,           /* File opened that is not a database file */
  NOTICE = 100,     /* Notifications from log() */

  /* buffer pool part */
  BUFFERPOOL_EXIST = (BUFFERPOOL | (RCBufferPool::BP_EXIST << 8)),
  BUFFERPOOL_FILEERR = (BUFFERPOOL | (RCBufferPool::BP_FILEERR << 8)),
  BUFFERPOOL_INVALIDNAME = (BUFFERPOOL | (RCBufferPool::BP_INVALIDNAME << 8)),
  BUFFERPOOL_WINDOWS = (BUFFERPOOL | (RCBufferPool::BP_WINDOWS << 8)),
  BUFFERPOOL_CLOSED = (BUFFERPOOL | (RCBufferPool::BP_CLOSED << 8)),
  BUFFERPOOL_OPEN = (BUFFERPOOL | (RCBufferPool::BP_OPEN << 8)),
  BUFFERPOOL_NOBUF = (BUFFERPOOL | (RCBufferPool::BP_NOBUF << 8)),
  BUFFERPOOL_EOF = (BUFFERPOOL | (RCBufferPool::BP_EOF << 8)),
  BUFFERPOOL_INVALID_PAGE_NUM = (BUFFERPOOL | (RCBufferPool::BP_INVALIDPAGENUM << 8)),
  BUFFERPOOL_NOTINBUF = (BUFFERPOOL | (RCBufferPool::BP_NOTINBUF << 8)),
  BUFFERPOOL_PAGE_PINNED = (BUFFERPOOL | (RCBufferPool::BP_PAGE_PINNED << 8)),
  BUFFERPOOL_OPEN_TOO_MANY_FILES = (BUFFERPOOL | (RCBufferPool::BP_OPEN_TOO_MANY_FILES << 8)),
  BUFFERPOOL_ILLEGAL_FILE_ID = (BUFFERPOOL | (RCBufferPool::BP_ILLEGAL_FILE_ID << 8)),

  /* record part */
  RECORD_CLOSED = (RECORD | (RCRecord::RD_CLOSED << 8)),
  RECORD_OPENNED = (RECORD | (RCRecord::RD_OPENNED << 8)),
  RECORD_INVALIDRECSIZE = (RECORD | (RCRecord::RD_INVALIDRECSIZE << 8)),
  RECORD_INVALIDRID = (RECORD | (RCRecord::RD_INVALIDRID << 8)),
  RECORD_NOMORERECINMEM = (RECORD | (RCRecord::RD_NO_MORE_RECINMEM << 8)),
  RECORD_OPEN = (RECORD | (RCRecord::RD_OPEN << 8)),
  RECORD_NO_MORE_IDX_IN_MEM = (RECORD | (RCRecord::RD_NO_MORE_IDXINMEM << 8)),
  RECORD_INVALID_KEY = (RECORD | (RCRecord::RD_INVALID_KEY << 8)),
  RECORD_DUPLICATE_KEY = (RECORD | (RCRecord::RD_DUPLICATE_KEY << 8)),
  RECORD_NOMEM = (RECORD | (RCRecord::RD_NOMEM << 8)),
  RECORD_SCANCLOSED = (RECORD | (RCRecord::RD_SCANCLOSED << 8)),
  RECORD_SCANOPENNED = (RECORD | (RCRecord::RD_SCANOPENNED << 8)),
  RECORD_EOF = (RECORD | (RCRecord::RD_EOF << 8)),
  RECORD_RECORD_NOT_EXIST = (RECORD | (RCRecord::RD_NOT_EXIST << 8)),

  /* schema part */
  SCHEMA_DB_EXIST = (SCHEMA | (RCSchema::DB_EXIST << 8)),
  SCHEMA_DB_NOT_EXIST = (SCHEMA | (RCSchema::DB_NOT_EXIST << 8)),
  SCHEMA_DB_NOT_OPENED = (SCHEMA | (RCSchema::DB_NOT_OPENED << 8)),
  SCHEMA_TABLE_NOT_EXIST = (SCHEMA | (RCSchema::TABLE_NOT_EXIST << 8)),
  SCHEMA_TABLE_EXIST = (SCHEMA | (RCSchema::TABLE_EXIST << 8)),
  SCHEMA_TABLE_NAME_ILLEGAL = (SCHEMA | (RCSchema::TABLE_NAME_ILLEGAL << 8)),
  SCHEMA_FIELD_NOT_EXIST = (SCHEMA | (RCSchema::FIELD_NOT_EXIST << 8)),
  SCHEMA_FIELD_EXIST = (SCHEMA | (RCSchema::FIELD_EXIST << 8)),
  SCHEMA_FIELD_NAME_ILLEGAL = (SCHEMA | (RCSchema::FIELD_NAME_ILLEGAL << 8)),
  SCHEMA_FIELD_MISSING = (SCHEMA | (RCSchema::FIELD_MISSING << 8)),
  SCHEMA_FIELD_REDUNDAN = (SCHEMA | (RCSchema::FIELD_REDUNDAN << 8)),
  SCHEMA_FIELD_TYPE_MISMATCH = (SCHEMA | (RCSchema::FIELD_TYPE_MISMATCH << 8)),
  SCHEMA_INDEX_NAME_REPEAT = (SCHEMA | (RCSchema::INDEX_NAME_REPEAT << 8)),
  SCHEMA_INDEX_EXIST = (SCHEMA | (RCSchema::INDEX_EXIST << 8)),
  SCHEMA_INDEX_NOT_EXIST = (SCHEMA | (RCSchema::INDEX_NOT_EXIST << 8)),
  SCHEMA_INDEX_NAME_ILLEGAL = (SCHEMA | (RCSchema::INDEX_NAME_ILLEGAL << 8)),

  /*ioerror part*/
  IOERR_READ = (IOERR | (RCIOError::READ << 8)),
  IOERR_SHORT_READ = (IOERR | (RCIOError::SHORT_READ << 8)),
  IOERR_WRITE = (IOERR | (RCIOError::WRITE << 8)),
  IOERR_FSYNC = (IOERR | (RCIOError::FSYNC << 8)),
  IOERR_DIR_FSYNC = (IOERR | (RCIOError::DIR_FSYNC << 8)),
  IOERR_TRUNCATE = (IOERR | (RCIOError::TRUNCATE << 8)),
  IOERR_FSTAT = (IOERR | (RCIOError::FSTAT << 8)),
  IOERR_DELETE = (IOERR | (RCIOError::IO_DELETE << 8)),
  IOERR_BLOCKED = (IOERR | (RCIOError::BLOCKED << 8)),
  IOERR_ACCESS = (IOERR | (RCIOError::ACCESS << 8)),
  IOERR_CHECKRESERVEDLOCK = (IOERR | (RCIOError::CHECKRESERVEDLOCK << 8)),
  IOERR_CLOSE = (IOERR | (RCIOError::CLOSE << 8)),
  IOERR_DIR_CLOSE = (IOERR | (RCIOError::DIR_CLOSE << 8)),
  IOERR_SHMOPEN = (IOERR | (RCIOError::SHMOPEN << 8)),
  IOERR_SHMSIZE = (IOERR | (RCIOError::SHMSIZE << 8)),
  IOERR_SHMLOCK = (IOERR | (RCIOError::SHMLOCK << 8)),
  IOERR_SHMMAP = (IOERR | (RCIOError::SHMMAP << 8)),
  IOERR_SEEK = (IOERR | (RCIOError::SEEK << 8)),
  IOERR_DELETE_NOENT = (IOERR | (RCIOError::DELETE_NOENT << 8)),
  IOERR_MMAP = (IOERR | (RCIOError::MMAP << 8)),
  IOERR_GETTEMPPATH = (IOERR | (RCIOError::GETTEMPPATH << 8)),
  IOERR_CONVPATH = (IOERR | (RCIOError::IO_CONVPATH << 8)),
  IOERR_VNODE = (IOERR | (RCIOError::VNODE << 8)),
  IOERR_BEGIN_ATOMIC = (IOERR | (RCIOError::BEGIN_ATOMIC << 8)),
  IOERR_COMMIT_ATOMIC = (IOERR | (RCIOError::COMMIT_ATOMIC << 8)),
  IOERR_ROLLBACK_ATOMIC = (IOERR | (RCIOError::ROLLBACK_ATOMIC << 8)),
  IOERR_DATA = (IOERR | (RCIOError::DATA << 8)),
  IOERR_CORRUPTFS = (IOERR | (RCIOError::CORRUPTFS << 8)),
  IOERR_OPEN_TOO_MANY_FILES = (IOERR | RCIOError::OPEN_TOO_MANY_FILES << 8),

  /* Lock part*/
  LOCKED_LOCK = (LOCKED | (RCLock::LOCK << 8)),
  LOCKED_UNLOCK = (LOCKED | (RCLock::UNLOCK << 8)),
  LOCKED_SHAREDCACHE = (LOCKED | (RCLock::SHAREDCACHE << 8)),
  LOCKED_VIRT = (LOCKED | (RCLock::LVIRT << 8)),
  LOCKED_NEED_WAIT = (LOCKED | (RCLock::NEED_WAIT << 8)),
  LOCKED_RESOURCE_DELETED = (LOCKED | (RCLock::RESOURCE_DELETED << 8)),

  /* busy part */
  BUSY_RECOVERY = (BUSY | (RCBusy::BRECOVERY << 8)),
  BUSY_SNAPSHOT = (BUSY | (RCBusy::SNAPSHOT << 8)),
  BUSY_TIMEOUT = (BUSY | (RCBusy::TIMEOUT << 8)),

  /* Can't open part */
  CANTOPEN_NOTEMPDIR = (CANTOPEN | (RCCantOpen::NOTEMPDIR << 8)),
  CANTOPEN_ISDIR = (CANTOPEN | (RCCantOpen::ISDIR << 8)),
  CANTOPEN_FULLPATH = (CANTOPEN | (RCCantOpen::FULLPATH << 8)),
  CANTOPEN_CONVPATH = (CANTOPEN | (RCCantOpen::CO_CONVPATH << 8)),
  CANTOPEN_DIRTYWAL = (CANTOPEN | (RCCantOpen::DIRTYWAL << 8)),
  CANTOPEN_SYMLINK = (CANTOPEN | (RCCantOpen::SYMLINK << 8)),

  /* corrupt part */  // compile error
  // CORRUPT_VIRT = (CORRUPT | (RCCorrupt::CORRUPT_VIRT << 8)),
  // CORRUPT_SEQUENCE = (CORRUPT | (RCCorrupt::CORRUPT_SEQUENCE << 8)),
  // CORRUPT_INDEX = (CORRUPT | (RCCorrupt::CORRUPT_INDEX << 8)),

  /*readonly part*/
  READONLY_RECOVERY = (READONLY | (RCReadonly::RO_RECOVERY << 8)),
  READONLY_CANTLOCK = (READONLY | (RCReadonly::CANTLOCK << 8)),
  READONLY_ROLLBACK = (READONLY | (RCReadonly::RO_ROLLBACK << 8)),
  READONLY_DBMOVED = (READONLY | (RCReadonly::DBMOVED << 8)),
  READONLY_CANTINIT = (READONLY | (RCReadonly::CANTINIT << 8)),
  READONLY_DIRECTORY = (READONLY | (RCReadonly::DIRECTORY << 8)),

  ABORT_ROLLBACK = (ABORT | (RCAbort::AROLLBACK << 8)),

  /* contraint part */
  CONSTRAINT_CHECK = (CONSTRAINT | (RCContraint::CHECK << 8)),
  CONSTRAINT_COMMITHOOK = (CONSTRAINT | (RCContraint::COMMITHOOK << 8)),
  CONSTRAINT_FOREIGNKEY = (CONSTRAINT | (RCContraint::FOREIGNKEY << 8)),
  CONSTRAINT_FUNCTION = (CONSTRAINT | (RCContraint::FUNCTION << 8)),
  CONSTRAINT_NOTNULL = (CONSTRAINT | (RCContraint::NOTNULL << 8)),
  CONSTRAINT_PRIMARYKEY = (CONSTRAINT | (RCContraint::PRIMARYKEY << 8)),
  CONSTRAINT_TRIGGER = (CONSTRAINT | (RCContraint::TRIGGER << 8)),
  CONSTRAINT_UNIQUE = (CONSTRAINT | (RCContraint::UNIQUE << 8)),
  CONSTRAINT_VIRT = (CONSTRAINT | (RCContraint::CVIRT << 8)),
  CONSTRAINT_ROWID = (CONSTRAINT | (RCContraint::ROWID << 8)),
  CONSTRAINT_PINNED = (CONSTRAINT | (RCContraint::PINNED << 8)),

  /* notic part */
  NOTICE_RECOVER_WAL = (NOTICE | (RCNotice::RECOVER_WAL << 8)),
  NOTICE_RECOVER_ROLLBACK = (NOTICE | (RCNotice::RECOVER_ROLLBACK << 8)),
  NOTICE_AUTOINDEX = (NOTICE | (RCNotice::AUTOINDEX << 8)),

  /* auth part*/
  AUTH_USER = (AUTH | (RCAuth::USER << 8)),
};

extern const char *strrc(RC rc);

#endif  //__OBSERVER_RC_H__
