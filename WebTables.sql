CREATE TABLE Balance(
  `username` varchar(100) NOT NULL,
  `balance` float NOT NULL,
  `currency` bool NOT NULL,
  PRIMARY KEY (username).
);

CREATE TABLE `Transactions` (
  `id` int NOT NULL AUTO_INCREMENT,
  `sender` varchar(100) NOT NULL,
  `receiver` varchar(100) NOT NULL,
  `amount` float NOT NULL,
  `currency_kind` bool NOT NULL DEFAULT 0,
  `publish_date` date DEFAULT curdate(),
  PRIMARY KEY (`id`),
  CONSTRAINT `Transactions_sender_FK` FOREIGN KEY (`sender`) REFERENCES `Balance` (`username`),
  CONSTRAINT `Transactions_receiver_FK` FOREIGN KEY (`receiver`) REFERENCES `Balance` (`username`)
);

INSERT INTO Balance(username, balance, currency) VALUES ('emilia.viquez', '100', true);
INSERT INTO Balance(username, balance, currency) VALUES ('brandon.moraumana', '100', true);
INSERT INTO Balance(username, balance, currency) VALUES ('esteban.leonrodriguez', '100', false);
INSERT INTO Balance(username, balance, currency) VALUES ('carlos.sanchezblanco', '100', false);
INSERT INTO Balance(username, balance, currency) VALUES ('maria.andres', '100', false);
INSERT INTO Balance(username, balance, currency) VALUES ('gerardo.camposbadilla', '100', true);
INSERT INTO Balance(username, balance, currency) VALUES ('randall.lopezvarela', '100', true);
INSERT INTO Balance(username, balance, currency) VALUES ('carlos.ramirezmasis', '100', false);
INSERT INTO Balance(username, balance, currency) VALUES ('david.sanchezlopez', '100', false);
INSERT INTO Balance(username, balance, currency) VALUES ('cristian.ortegahurtado', '100', false);
INSERT INTO Balance(username, balance, currency) VALUES ('andre.villegas', '100', true);
INSERT INTO Balance(username, balance, currency) VALUES ('andrey.menaespinoza', '100', true);
INSERT INTO Balance(username, balance, currency) VALUES ('jason.murillo', '100', false);
INSERT INTO Balance(username, balance, currency) VALUES ('genesis.herreraknyght', '100', false);
INSERT INTO Balance(username, balance, currency) VALUES ('yordi.robles', '100', false);
INSERT INTO Balance(username, balance, currency) VALUES ('sebastian.rodrigueztencio', '100', true);
INSERT INTO Balance(username, balance, currency) VALUES ('jeremy.espinozamadrigal', '100', true);
INSERT INTO Balance(username, balance, currency) VALUES ('richard.garita', '100', false);
INSERT INTO Balance(username, balance, currency) VALUES ('dylan.tenorio', '100', false);
INSERT INTO Balance(username, balance, currency) VALUES ('eithel.vega', '100', false);