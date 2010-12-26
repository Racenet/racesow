-- phpMyAdmin SQL Dump
-- version 3.2.2.1
-- http://www.phpmyadmin.net
--
-- Serveur: localhost
-- Généré le : Lun 11 Octobre 2010 à 23:16
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
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 AUTO_INCREMENT=2 ;

-- --------------------------------------------------------

--
-- Structure de la table `map`
--

DROP TABLE IF EXISTS `map`;
CREATE TABLE `map` (
  `id` mediumint(8) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(40) DEFAULT NULL,
  `longname` varchar(64) DEFAULT NULL,
  `file` varchar(255) DEFAULT NULL,
  `oneliner` varchar(255) DEFAULT NULL,
  `mapper_id` mediumint(8) unsigned DEFAULT NULL,
  `freestyle` tinyint(1) NOT NULL DEFAULT '0',
  `status` enum('enabled','disabled','new','true','false') NOT NULL DEFAULT 'new',
  `races` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `playtime` bigint(20) unsigned NOT NULL DEFAULT '0',
  `rating` float unsigned DEFAULT NULL,
  `ratings` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `downloads` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `force_recompution` enum('true','false') NOT NULL DEFAULT 'false',
  `weapons` varchar(10) NOT NULL DEFAULT '0000000',
  `created` datetime DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `races` (`races`),
  KEY `playtime` (`playtime`),
  KEY `name` (`name`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 AUTO_INCREMENT=6 ;

-- --------------------------------------------------------

--
-- Structure de la table `player`
--

DROP TABLE IF EXISTS `player`;
CREATE TABLE `player` (
  `id` mediumint(8) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(64) NOT NULL DEFAULT '',
  `simplified` varchar(64) NOT NULL DEFAULT '',
  `auth_name` varchar(255) DEFAULT NULL,
  `auth_token` varchar(255) DEFAULT NULL,
  `auth_email` varchar(255) DEFAULT NULL,
  `auth_mask` varchar(255) NOT NULL DEFAULT '0',
  `auth_pass` varchar(255) DEFAULT NULL,
  `session_token` varchar(255) DEFAULT NULL,
  `points` int(11) NOT NULL DEFAULT '0',
  `races` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `maps` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `diff_points` mediumint(9) DEFAULT NULL,
  `awardval` mediumint(8) unsigned DEFAULT NULL,
  `playtime` bigint(20) NOT NULL DEFAULT '0',
  `created` datetime DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `points` (`points`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 AUTO_INCREMENT=2 ;

-- --------------------------------------------------------

--
-- Structure de la table `checkpoint`
--

DROP TABLE IF EXISTS `checkpoint`;
CREATE TABLE `checkpoint` (
  `player_id` int(11) NOT NULL,
  `map_id` int(11) NOT NULL,
  `num` int(11) unsigned NOT NULL,
  `time` int(11) NOT NULL,
  PRIMARY KEY (`map_id`,`player_id`,`num`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Structure de la table `player_history`
--

DROP TABLE IF EXISTS `player_history`;
CREATE TABLE `player_history` (
  `player_id` mediumint(8) unsigned NOT NULL,
  `date` date NOT NULL,
  `points` int(11) NOT NULL DEFAULT '0',
  `races` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `maps` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `skill` float NOT NULL DEFAULT '0',
  `awardval` mediumint(8) unsigned DEFAULT NULL,
  `playtime` bigint(20) NOT NULL DEFAULT '0',
  `created` datetime DEFAULT NULL,
  PRIMARY KEY (`player_id`,`date`),
  KEY `points` (`points`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Structure de la table `player_map`
--

DROP TABLE IF EXISTS `player_map`;
CREATE TABLE `player_map` (
  `player_id` int(11) unsigned NOT NULL,
  `map_id` int(11) unsigned NOT NULL,
  `server_id` int(11) DEFAULT NULL,
  `time` int(11) unsigned DEFAULT NULL,
  `races` int(11) unsigned NOT NULL DEFAULT '0',
  `points` int(11) unsigned NOT NULL DEFAULT '0',
  `playtime` bigint(20) unsigned NOT NULL DEFAULT '0',
  `tries` int(11) DEFAULT NULL,
  `duration` bigint(20) DEFAULT NULL,
  `overall_tries` int(11) NOT NULL DEFAULT '0',
  `racing_time` bigint(20) NOT NULL DEFAULT '0',
  `created` datetime DEFAULT NULL,
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
  `server_id` int(10) unsigned DEFAULT NULL,
  `time` int(11) unsigned NOT NULL DEFAULT '0',
  `tries` int(10) unsigned DEFAULT NULL,
  `duration` bigint(20) DEFAULT NULL,
  `created` datetime DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 AUTO_INCREMENT=6 ;
