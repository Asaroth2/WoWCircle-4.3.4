ALTER TABLE `account_banned`
ADD COLUMN `realm`  int(11) NOT NULL AFTER `id`;

DELETE FROM `trinity_string` WHERE `entry` IN ('11107', '11108');
INSERT INTO `trinity_string` (`entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`) VALUES ('11107','|cffff0000[GM]:|r %s was banned by %s for %s. Reason: %s.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,'|cffff0000[GM]:|r %s ��� ������� %s �� %s. �������: %s.');
INSERT INTO `trinity_string` (`entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`) VALUES ('11108','|cffff0000[GM]:|r %s was banned by %s for eternity. Reason: %s.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,'|cffff0000[GM]:|r %s ��� ������� %s ��������. �������: %s.');

