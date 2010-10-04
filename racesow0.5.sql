-- phpMyAdmin SQL Dump
-- version 3.2.2.1
-- http://www.phpmyadmin.net
--
-- Serveur: localhost
-- Généré le : Lun 04 Octobre 2010 à 12:01
-- Version du serveur: 5.1.50
-- Version de PHP: 5.3.3

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

--
-- Base de données: `racesow`
--

-- --------------------------------------------------------

--
-- Structure de la table `gameserver`
--

DROP TABLE IF EXISTS `gameserver`;
CREATE TABLE `gameserver` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `user` varchar(255) NOT NULL,
  `servername` varchar(255) DEFAULT NULL,
  `admin` varchar(255) DEFAULT NULL,
  `playtime` bigint(20) unsigned NOT NULL DEFAULT '0',
  `races` int(10) unsigned NOT NULL DEFAULT '0',
  `maps` int(10) unsigned NOT NULL DEFAULT '0',
  `created` datetime NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Structure de la table `map`
--

DROP TABLE IF EXISTS `map`;
CREATE TABLE `map` (
  `id` mediumint(8) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(40) NOT NULL DEFAULT '',
  `freestyle` tinyint(1) NOT NULL DEFAULT '0',
  `status` enum('enabled','disabled','new','true','false') NOT NULL DEFAULT 'new',
  `disabled` enum('true','false') NOT NULL DEFAULT 'false',
  `races` mediumint(8) unsigned NOT NULL,
  `playtime` bigint(20) unsigned NOT NULL,
  `weapons` varchar(10) NOT NULL,
  `created` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `races` (`races`),
  KEY `playtime` (`playtime`),
  KEY `name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Structure de la table `map_checkpoint`
--

DROP TABLE IF EXISTS `map_checkpoint`;
CREATE TABLE `map_checkpoint` (
  `map_id` int(11) NOT NULL,
  `num` int(11) unsigned NOT NULL,
  `time` int(11) NOT NULL,
  PRIMARY KEY (`map_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Structure de la table `player`
--

DROP TABLE IF EXISTS `player`;
CREATE TABLE `player` (
  `id` mediumint(8) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(64) NOT NULL DEFAULT '',
  `simplified` varchar(64) NOT NULL DEFAULT '',
  `auth_name` varchar(255) NOT NULL,
  `auth_token` varchar(255) NOT NULL,
  `auth_email` varchar(255) NOT NULL,
  `auth_mask` varchar(255) NOT NULL,
  `auth_pass` varchar(255) NOT NULL,
  `session_token` varchar(255) DEFAULT NULL,
  `points` int(11) NOT NULL DEFAULT '0',
  `position` mediumint(8) unsigned DEFAULT NULL,
  `races` mediumint(8) unsigned NOT NULL,
  `maps` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `playtime` bigint(20) NOT NULL,
  `created` datetime DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `points` (`points`),
  KEY `position` (`position`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Structure de la table `player_checkpoint`
--

DROP TABLE IF EXISTS `player_checkpoint`;
CREATE TABLE `player_checkpoint` (
  `player_id` int(11) NOT NULL,
  `map_id` int(11) NOT NULL,
  `num` int(11) unsigned NOT NULL,
  `time` int(11) NOT NULL,
  PRIMARY KEY (`map_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Structure de la table `player_map`
--

DROP TABLE IF EXISTS `player_map`;
CREATE TABLE `player_map` (
  `player_id` int(11) unsigned NOT NULL,
  `map_id` int(11) unsigned NOT NULL,
  `time` int(11) unsigned DEFAULT NULL,
  `races` int(11) unsigned NOT NULL DEFAULT '0',
  `points` int(11) unsigned NOT NULL DEFAULT '0',
  `playtime` bigint(20) unsigned NOT NULL DEFAULT '0',
  `server_id` int(11) DEFAULT NULL,
  `created` datetime DEFAULT NULL,
  `tries` int(11) DEFAULT NULL,
  `duration` bigint(20) DEFAULT NULL,
  `overall_tries` int(11) DEFAULT NULL,
  `racing_time` bigint(20) DEFAULT NULL,
  PRIMARY KEY (`player_id`,`map_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Structure de la table `race`
--

DROP TABLE IF EXISTS `race`;
CREATE TABLE `race` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `map_id` int(11) unsigned NOT NULL DEFAULT '0',
  `player_id` int(11) unsigned NOT NULL DEFAULT '0',
  `nick_id` int(11) unsigned NOT NULL DEFAULT '0',
  `time` int(11) unsigned NOT NULL DEFAULT '0',
  `tries` int(10) unsigned DEFAULT NULL,
  `duration` bigint(20) DEFAULT NULL,
  `server_id` int(10) unsigned DEFAULT NULL,
  `created` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;
