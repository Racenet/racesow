-- phpMyAdmin SQL Dump
-- version 2.11.8.1deb5+lenny6
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Erstellungszeit: 08. Oktober 2010 um 17:56
-- Server Version: 5.0.51
-- PHP-Version: 5.2.6-1+lenny9

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

--
-- Datenbank: `racesow056`
--

-- --------------------------------------------------------

--
-- Tabellenstruktur für Tabelle `gameserver`
--

DROP TABLE IF EXISTS `gameserver`;
CREATE TABLE `gameserver` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `user` varchar(255) NOT NULL,
  `servername` varchar(255) default NULL,
  `admin` varchar(255) default NULL,
  `playtime` bigint(20) unsigned NOT NULL default '0',
  `races` int(10) unsigned NOT NULL default '0',
  `maps` int(10) unsigned NOT NULL default '0',
  `created` datetime NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Tabellenstruktur für Tabelle `map`
--

DROP TABLE IF EXISTS `map`;
CREATE TABLE `map` (
  `id` mediumint(8) unsigned NOT NULL auto_increment,
  `name` varchar(40) default NULL,
  `longname` varchar(64) default NULL,
  `file` varchar(255) default NULL,
  `mapper_id` mediumint(8) unsigned default NULL,
  `freestyle` tinyint(1) NOT NULL default '0',
  `status` enum('enabled','disabled','new','true','false') NOT NULL default 'new',
  `disabled` enum('true','false') NOT NULL default 'false',
  `races` mediumint(8) unsigned NOT NULL default '0',
  `playtime` bigint(20) unsigned NOT NULL default '0',
  `rating` float unsigned default NULL,
  `ratings` mediumint(8) unsigned NOT NULL default '0',
  `downloads` mediumint(8) unsigned NOT NULL default '0',
  `force_recompution` enum('true','false') NOT NULL default 'false',
  `weapons` varchar(10) NOT NULL default '0000000',
  `created` datetime default NULL,
  PRIMARY KEY  (`id`),
  KEY `races` (`races`),
  KEY `playtime` (`playtime`),
  KEY `name` (`name`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Tabellenstruktur für Tabelle `player`
--

DROP TABLE IF EXISTS `player`;
CREATE TABLE `player` (
  `id` mediumint(8) unsigned NOT NULL auto_increment,
  `name` varchar(64) NOT NULL default '',
  `simplified` varchar(64) NOT NULL default '',
  `auth_name` varchar(255) default NULL,
  `auth_token` varchar(255) default NULL,
  `auth_email` varchar(255) default NULL,
  `auth_mask` varchar(255) NOT NULL default '0',
  `auth_pass` varchar(255) default NULL,
  `session_token` varchar(255) default NULL,
  `points` int(11) NOT NULL default '0',
  `races` mediumint(8) unsigned NOT NULL default '0',
  `maps` mediumint(8) unsigned NOT NULL default '0',
  `diff_points` mediumint(9) default NULL,
  `awardval` mediumint(8) unsigned default NULL,
  `playtime` bigint(20) NOT NULL default '0',
  `created` datetime default NULL,
  PRIMARY KEY  (`id`),
  KEY `points` (`points`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Tabellenstruktur für Tabelle `player_history`
--

DROP TABLE IF EXISTS `player_history`;
CREATE TABLE `player_history` (
  `player_id` mediumint(8) unsigned NOT NULL,
  `date` date NOT NULL,
  `points` int(11) NOT NULL default '0',
  `races` mediumint(8) unsigned NOT NULL default '0',
  `maps` mediumint(8) unsigned NOT NULL default '0',
  `skill` float NOT NULL default '0',
  `awardval` mediumint(8) unsigned default NULL,
  `playtime` bigint(20) NOT NULL default '0',
  `created` datetime default NULL,
  PRIMARY KEY  (`player_id`,`date`),
  KEY `points` (`points`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Tabellenstruktur für Tabelle `player_map`
--

DROP TABLE IF EXISTS `player_map`;
CREATE TABLE `player_map` (
  `player_id` int(11) unsigned NOT NULL,
  `map_id` int(11) unsigned NOT NULL,
  `server_id` int(11) default NULL,
  `time` int(11) unsigned default NULL,
  `races` int(11) unsigned NOT NULL default '0',
  `points` int(11) unsigned NOT NULL default '0',
  `playtime` bigint(20) unsigned NOT NULL default '0',
  `tries` int(11) default NULL,
  `duration` bigint(20) default NULL,
  `overall_tries` int(11) NOT NULL,
  `racing_time` bigint(20) NOT NULL,
  `created` datetime default NULL,
  PRIMARY KEY  (`player_id`,`map_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Tabellenstruktur für Tabelle `race`
--

DROP TABLE IF EXISTS `race`;
CREATE TABLE `race` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `map_id` int(11) unsigned NOT NULL default '0',
  `player_id` int(11) unsigned NOT NULL default '0',
  `nick_id` int(11) unsigned NOT NULL default '0',
  `server_id` int(10) unsigned default NULL,
  `time` int(11) unsigned NOT NULL default '0',
  `tries` int(10) unsigned default NULL,
  `duration` bigint(20) default NULL,
  `created` datetime default NULL,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;
