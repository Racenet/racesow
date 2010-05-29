CREATE TABLE `map` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `name` varchar(40) NOT NULL default '',
  `freestyle` enum('true','false') NOT NULL default 'false',
  `status` enum('enabled','disabled') NOT NULL default 'enabled',
  `races` int(11) unsigned NOT NULL,
  `playtime` bigint(20) unsigned NOT NULL,
  `rating` tinyint(1) NOT NULL default '0',
  `ratings` int(11) NOT NULL default '0',
  `created` timestamp NOT NULL default CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

CREATE TABLE `map_rating` (
  `map_id` int(11) unsigned NOT NULL,
  `player_id` int(11) unsigned NOT NULL,
  `value` tinyint(3) unsigned NOT NULL,
  `created` datetime NOT NULL default '0000-00-00 00:00:00',
  `changed` datetime default NULL,
  UNIQUE KEY `map_id` (`map_id`,`player_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

CREATE TABLE `nick` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `name` varchar(64) NOT NULL default '',
  `simplified` varchar(64) NOT NULL default '',
  `races` int(11) unsigned NOT NULL,
  `maps` int(11) unsigned NOT NULL default '0',
  `playtime` bigint(20) NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

CREATE TABLE `player` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `auth_name` varchar(64) NOT NULL default '',
  `auth_pass` varchar(32) NOT NULL default '',
  `auth_mask` int(11) NOT NULL default '0',
  `name` varchar(64) NOT NULL default '',
  `simplified` varchar(64) NOT NULL default '',
  `points` int(11) NOT NULL default '0',
  `races` int(11) unsigned NOT NULL,
  `maps` int(11) unsigned NOT NULL default '0',
  `diff_points` mediumint(9) NOT NULL,
  `playtime` bigint(20) NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

CREATE TABLE `player_map` (
  `player_id` int(11) unsigned NOT NULL,
  `map_id` int(11) unsigned NOT NULL,
  `time` int(11) unsigned default NULL,
  `races` int(11) unsigned NOT NULL default '0',
  `points` int(11) unsigned NOT NULL default '0',
  `playtime` bigint(20) unsigned NOT NULL default '0',
  `created` datetime default NULL,
  PRIMARY KEY  (`player_id`,`map_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

CREATE TABLE `race` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `map_id` int(11) unsigned NOT NULL default '0',
  `player_id` int(11) unsigned NOT NULL default '0',
  `nick_id` int(11) unsigned NOT NULL default '0',
  `time` int(11) unsigned NOT NULL default '0',
  `created` datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
