/*
 * Syslog JSON message writer.
 *
 * Copyright (C) 2015 Red Hat
 *
 * This file is part of tlog.
 *
 * Tlog is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Tlog is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tlog; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <tlog/journal_json_writer.h>
#include <systemd/sd-journal.h>
#include <tlog/grc.h>
#include <tlog/rc.h>

/** Syslog writer data */
struct tlog_journal_json_writer {
    struct tlog_json_writer     writer;     /**< Abstract writer instance */
    int                         priority;   /**< Logging priority */
    char                       *username;   /**< Session user name */
    unsigned int                session_id; /**< Session ID */
};

static tlog_grc
tlog_journal_json_writer_init(struct tlog_json_writer *writer, va_list ap)
{
    struct tlog_journal_json_writer *journal_json_writer =
                                    (struct tlog_journal_json_writer*)writer;
    journal_json_writer->priority = va_arg(ap, int);
    journal_json_writer->username = strdup(va_arg(ap, const char *));
    if (journal_json_writer->username == NULL) {
        return TLOG_GRC_ERRNO;
    }
    journal_json_writer->session_id = va_arg(ap, unsigned int);
    return TLOG_RC_OK;
}

static bool
tlog_journal_json_writer_is_valid(const struct tlog_json_writer *writer)
{
    struct tlog_journal_json_writer *journal_json_writer =
                                    (struct tlog_journal_json_writer*)writer;
    return tlog_syslog_priority_is_valid(journal_json_writer->priority) &&
           journal_json_writer->username != NULL &&
           journal_json_writer->session_id != 0;
}

static void
tlog_journal_json_writer_cleanup(struct tlog_json_writer *writer)
{
    struct tlog_journal_json_writer *journal_json_writer =
                                    (struct tlog_journal_json_writer*)writer;
    free(journal_json_writer->username);
}

static tlog_grc
tlog_journal_json_writer_write(struct tlog_json_writer *writer,
                               size_t id, const uint8_t *buf, size_t len)
{
    struct tlog_journal_json_writer *journal_json_writer =
                                    (struct tlog_journal_json_writer*)writer;
    int sd_rc;
    sd_rc = sd_journal_send("PRIORITY=%d", journal_json_writer->priority,
                            "TLOG_USER=%s", journal_json_writer->username,
                            "TLOG_SESSION=%u", journal_json_writer->session_id,
                            "TLOG_ID=%zu", id,
                            "MESSAGE=%.*s", len, buf,
                            NULL);
    return (sd_rc < 0) ? TLOG_GRC_FROM(systemd, sd_rc) : TLOG_RC_OK;
}

const struct tlog_json_writer_type tlog_journal_json_writer_type = {
    .size       = sizeof(struct tlog_journal_json_writer),
    .init       = tlog_journal_json_writer_init,
    .cleanup    = tlog_journal_json_writer_cleanup,
    .is_valid   = tlog_journal_json_writer_is_valid,
    .write      = tlog_journal_json_writer_write,
};
