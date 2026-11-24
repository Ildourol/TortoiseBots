-- Clear travel-node tables built under swapped navmesh tile coordinates so
-- the next server start rebuilds a complete travel graph automatically.
DELETE FROM `ai_playerbot_travelnode_path`;
DELETE FROM `ai_playerbot_travelnode_link`;
DELETE FROM `ai_playerbot_travelnode`;
