-- The Restful Matching-Engine.
-- Copyright (C) 2013, 2017 Swirly Cloud Limited.
--
-- This program is free software; you can redistribute it and/or modify it under the terms of the
-- GNU General Public License as published by the Free Software Foundation; either version 2 of the
-- License, or (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
-- even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
-- General Public License for more details.
--
-- You should have received a copy of the GNU General Public License along with this program; if
-- not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
-- 02110-1301, USA.

-- Use ';' on single line to terminate statement.
PRAGMA foreign_keys = ON
;

BEGIN TRANSACTION
;

CREATE TABLE state_t (
  id INT NOT NULL PRIMARY KEY,
  symbol CHAR(16) NOT NULL UNIQUE
)
;

INSERT INTO state_t (id, symbol) VALUES (1, 'NEW')
;
INSERT INTO state_t (id, symbol) VALUES (2, 'REVISE')
;
INSERT INTO state_t (id, symbol) VALUES (3, 'CANCEL')
;
INSERT INTO state_t (id, symbol) VALUES (4, 'TRADE')
;

CREATE TABLE side_t (
  id INT NOT NULL PRIMARY KEY,
  symbol CHAR(16) NOT NULL UNIQUE
)
;

INSERT INTO side_t (id, symbol) VALUES (1, 'BUY')
;
INSERT INTO side_t (id, symbol) VALUES (-1, 'SELL')
;

CREATE TABLE direct_t (
  id INT NOT NULL PRIMARY KEY,
  symbol CHAR(16) NOT NULL UNIQUE
)
;

INSERT INTO direct_t (id, symbol) VALUES (1, 'PAID')
;
INSERT INTO direct_t (id, symbol) VALUES (-1, 'GIVEN')
;

CREATE TABLE liqind_t (
  id INT NOT NULL PRIMARY KEY,
  symbol CHAR(16) NOT NULL UNIQUE
)
;

INSERT INTO liqind_t (id, symbol) VALUES (1, 'MAKER')
;
INSERT INTO liqind_t (id, symbol) VALUES (2, 'TAKER')
;

CREATE TABLE asset_type_t (
  id INT NOT NULL PRIMARY KEY,
  symbol CHAR(16) NOT NULL UNIQUE
)
;

INSERT INTO asset_type_t (id, symbol) VALUES (1, 'CMDTY')
;
INSERT INTO asset_type_t (id, symbol) VALUES (2, 'CORP')
;
INSERT INTO asset_type_t (id, symbol) VALUES (3, 'CCY')
;
INSERT INTO asset_type_t (id, symbol) VALUES (4, 'EQTY')
;
INSERT INTO asset_type_t (id, symbol) VALUES (5, 'GOVT')
;
INSERT INTO asset_type_t (id, symbol) VALUES (6, 'INDEX')
;

CREATE TABLE asset_t (
  id INT NOT NULL PRIMARY KEY,
  symbol CHAR(16) NOT NULL UNIQUE,
  display VARCHAR(64) NOT NULL UNIQUE,
  type_id INT NOT NULL,

  FOREIGN KEY (type_id) REFERENCES asset_type_t (id)
)
;

CREATE TABLE instr_t (
  id INT NOT NULL PRIMARY KEY,
  symbol CHAR(16) NOT NULL UNIQUE,
  display VARCHAR(64) NOT NULL UNIQUE,
  base_asset CHAR(16) NOT NULL,
  term_ccy CHAR(16) NOT NULL,
  lot_numer INT NOT NULL,
  lot_denom INT NOT NULL,
  tick_numer INT NOT NULL,
  tick_denom INT NOT NULL,
  pip_dp INT NOT NULL,
  min_lots BIGINT NOT NULL DEFAULT 1,
  max_lots BIGINT NOT NULL,

  FOREIGN KEY (base_asset) REFERENCES asset_t (symbol),
  FOREIGN KEY (term_ccy) REFERENCES asset_t (symbol)
)
;

CREATE TABLE accnt_t (
  symbol CHAR(16) NOT NULL PRIMARY KEY,
  max_id BIGINT NOT NULL DEFAULT 0,
  created BIGINT NOT NULL,
  modified BIGINT NOT NULL
)
;

CREATE INDEX accnt_modified_idx ON accnt_t (modified);

CREATE TABLE market_t (
  id BIGINT NOT NULL PRIMARY KEY,
  instr CHAR(16) NOT NULL,
  settl_day INT NULL DEFAULT NULL,
  state INT NOT NULL DEFAULT 0,
  last_lots BIGINT NULL DEFAULT NULL,
  last_ticks BIGINT NULL DEFAULT NULL,
  last_time BIGINT NULL DEFAULT NULL,

  FOREIGN KEY (instr) REFERENCES instr_t (symbol)
)
;

CREATE TABLE order_t (
  accnt CHAR(16) NOT NULL,
  market_id BIGINT NOT NULL,
  instr CHAR(16) NOT NULL,
  settl_day INT NULL DEFAULT NULL,
  id BIGINT NOT NULL,
  ref VARCHAR(64) NULL DEFAULT NULL,
  state_id INT NOT NULL,
  side_id INT NOT NULL,
  lots BIGINT NOT NULL,
  ticks BIGINT NOT NULL,
  resd_lots BIGINT NOT NULL,
  exec_lots BIGINT NOT NULL,
  exec_cost BIGINT NOT NULL,
  last_lots BIGINT NULL DEFAULT NULL,
  last_ticks BIGINT NULL DEFAULT NULL,
  min_lots BIGINT NOT NULL DEFAULT 1,
  created BIGINT NOT NULL,
  modified BIGINT NOT NULL,

  PRIMARY KEY (market_id, id),

  FOREIGN KEY (market_id) REFERENCES market_t (id),
  FOREIGN KEY (instr) REFERENCES instr_t (symbol),
  FOREIGN KEY (state_id) REFERENCES state_t (id),
  FOREIGN KEY (side_id) REFERENCES side_t (id)
)
;

CREATE INDEX order_resd_idx ON order_t (resd_lots);

CREATE TABLE exec_t (
  accnt CHAR(16) NOT NULL,
  market_id BIGINT NOT NULL,
  instr CHAR(16) NOT NULL,
  settl_day INT NULL DEFAULT NULL,
  id BIGINT NOT NULL,
  order_id BIGINT NULL DEFAULT NULL,
  ref VARCHAR(64) NULL DEFAULT NULL,
  seq_id BIGINT NULL DEFAULT NULL,
  state_id INT NOT NULL,
  side_id INT NOT NULL,
  lots BIGINT NOT NULL,
  ticks BIGINT NOT NULL,
  resd_lots BIGINT NOT NULL,
  exec_lots BIGINT NOT NULL,
  exec_cost BIGINT NOT NULL,
  last_lots BIGINT NULL DEFAULT NULL,
  last_ticks BIGINT NULL DEFAULT NULL,
  min_lots BIGINT NOT NULL DEFAULT 1,
  match_id BIGINT NULL DEFAULT NULL,
  liqind_id INT NULL DEFAULT NULL,
  cpty CHAR(16) NULL DEFAULT NULL,
  created BIGINT NOT NULL,
  archive BIGINT NULL DEFAULT NULL,

  PRIMARY KEY (market_id, id),

  FOREIGN KEY (market_id) REFERENCES market_t (id),
  FOREIGN KEY (market_id, order_id) REFERENCES order_t (market_id, id),
  FOREIGN KEY (instr) REFERENCES instr_t (symbol),
  FOREIGN KEY (state_id) REFERENCES state_t (id),
  FOREIGN KEY (side_id) REFERENCES side_t (id),
  FOREIGN KEY (liqind_id) REFERENCES liqind_t (id)
)
;

CREATE INDEX exec_accnt_seq_id_idx ON exec_t (accnt, seq_id);
CREATE INDEX exec_state_archive_idx ON exec_t (state_id, archive);

CREATE TRIGGER before_insert_on_exec1
  BEFORE INSERT ON exec_t
  WHEN NEW.order_id IS NOT NULL
  AND NEW.state_id = 1
  BEGIN
    INSERT INTO order_t (
      accnt,
      market_id,
      instr,
      settl_day,
      id,
      ref,
      state_id,
      side_id,
      lots,
      ticks,
      resd_lots,
      exec_lots,
      exec_cost,
      last_lots,
      last_ticks,
      min_lots,
      created,
      modified
    ) VALUES (
      NEW.accnt,
      NEW.market_id,
      NEW.instr,
      NEW.settl_day,
      NEW.order_id,
      NEW.ref,
      NEW.state_id,
      NEW.side_id,
      NEW.lots,
      NEW.ticks,
      NEW.resd_lots,
      NEW.exec_lots,
      NEW.exec_cost,
      NEW.last_lots,
      NEW.last_ticks,
      NEW.min_lots,
      NEW.created,
      NEW.created
    );
  END
;

CREATE TRIGGER before_insert_on_exec2
  BEFORE INSERT ON exec_t
  WHEN NEW.order_id IS NOT NULL
  AND NEW.state_id != 1
  BEGIN
    UPDATE order_t
    SET
      state_id = NEW.state_id,
      lots = NEW.lots,
      resd_lots = NEW.resd_lots,
      exec_lots = NEW.exec_lots,
      exec_cost = NEW.exec_cost,
      last_lots = NEW.last_lots,
      last_ticks = NEW.last_ticks,
      modified = NEW.created
    WHERE id = NEW.order_id;
  END
;

CREATE TRIGGER before_insert_on_exec3
  BEFORE INSERT ON exec_t
  WHEN NEW.state_id = 4
  BEGIN
    UPDATE market_t
    SET
      last_lots = NEW.last_lots,
      last_ticks = NEW.last_ticks,
      last_time = NEW.created
    WHERE id = NEW.market_id;
  END
;

CREATE TRIGGER after_insert_on_exec1
  AFTER INSERT ON exec_t
  BEGIN
    INSERT OR IGNORE INTO accnt_t (
      symbol,
      created,
      modified
    ) VALUES (
      NEW.accnt,
      NEW.created,
      NEW.created
    );
    UPDATE accnt_t
    SET
      max_id = max_id + 1,
      modified = NEW.created
    WHERE symbol = NEW.accnt;
    UPDATE exec_t
    SET
      seq_id = (SELECT max_id FROM accnt_t WHERE symbol = NEW.accnt)
    WHERE market_id = NEW.market_id
    AND id = NEW.id;
  END
;

CREATE VIEW asset_v AS
  SELECT
    a.id,
    a.symbol,
    a.display,
    t.symbol type
  FROM asset_t a
  LEFT OUTER JOIN asset_type_t t
  ON a.type_id = t.id
;

CREATE VIEW instr_v AS
  SELECT
    i.id,
    i.symbol,
    i.display,
    a.type,
    i.base_asset,
    i.term_ccy,
    i.lot_numer,
    i.lot_denom,
    i.tick_numer,
    i.tick_denom,
    i.pip_dp,
    i.min_lots,
    i.max_lots
  FROM instr_t i
  LEFT OUTER JOIN asset_v a
  ON i.base_asset = a.symbol
;

CREATE VIEW market_v AS
  SELECT
    m.id,
    m.instr,
    m.settl_day,
    m.state,
    m.last_lots,
    m.last_ticks,
    m.last_time,
    MAX(e.id) max_id
  FROM market_t m
  LEFT OUTER JOIN exec_t e
  ON m.id = e.market_id
  GROUP BY m.id
;

CREATE VIEW order_v AS
  SELECT
    o.accnt,
    o.market_id,
    o.instr,
    o.settl_day,
    o.id,
    o.ref,
    s.symbol state,
    a.symbol side,
    o.lots,
    o.ticks,
    o.resd_lots,
    o.exec_lots,
    o.exec_cost,
    o.last_lots,
    o.last_ticks,
    o.min_lots,
    o.created,
    o.modified
  FROM order_t o
  LEFT OUTER JOIN state_t s
  ON o.state_id = s.id
  LEFT OUTER JOIN side_t a
  ON o.side_id = a.id
;

CREATE VIEW exec_v AS
  SELECT
    e.accnt,
    e.market_id,
    e.instr,
    e.settl_day,
    e.id,
    e.order_id,
    e.ref,
    e.seq_id,
    s.symbol state,
    a.symbol side,
    e.lots,
    e.ticks,
    e.resd_lots,
    e.exec_lots,
    e.exec_cost,
    e.last_lots,
    e.last_ticks,
    e.min_lots,
    e.match_id,
    r.symbol liqind,
    e.cpty,
    e.created,
    e.archive
  FROM exec_t e
  LEFT OUTER JOIN state_t s
  ON e.state_id = s.id
  LEFT OUTER JOIN side_t a
  ON e.side_id = a.id
  LEFT OUTER JOIN liqind_t r
  ON e.liqind_id = r.id
;

CREATE VIEW posn_v AS
  SELECT
    e.accnt,
    e.market_id,
    e.instr,
    e.settl_day,
    e.side_id,
    SUM(e.last_lots) lots,
    SUM(e.last_lots * e.last_ticks) cost
  FROM exec_t e
  WHERE e.state_id = 4
  GROUP BY e.accnt, e.market_id, e.side_id
;

COMMIT
;
