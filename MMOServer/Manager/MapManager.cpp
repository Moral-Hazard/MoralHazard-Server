#include "pch.h"
#include "MapManager.hpp"
#include "Session/GameSession.hpp"
#include "Object/Player.hpp"
#include "Utility/Converter.hpp"

#include <filesystem>


MapManager::MapManager()
{
	for (auto& iter : std::filesystem::directory_iterator(TEXT("common/generated/maps/")))
	{
		auto map = MapData(action::Split(String(iter.path()), TEXT('/')).back());
		auto gameMap = std::make_shared<GameMap>(map);
		m_mapData[map.GetName()] = gameMap;
	}
}

std::shared_ptr<GameMap> MapManager::GetMap(String name)
{
	return m_mapData[name];
}

void MapManager::HandleEnter(std::shared_ptr<Session> session, gen::mmo::EnterMapReq packet)
{
	auto gameSession = std::static_pointer_cast<GameSession>(session);
	auto gameMap = m_mapData[packet.mapName];
	auto myPlayer = gameSession->GetPlayer();
	if (gameMap != nullptr && myPlayer->GetMap() != gameMap)
	{
		myPlayer->SetMap(gameMap);

		// send my position
		{
			gen::mmo::Spawn spawn;
			gen::mmo::PlayerInfo info;
			spawn.isMine = true;
			if (packet.position.x == -1 && packet.position.y == -1)
			{
				myPlayer->SetPosition(Vector2DI(1, 6));
				info.objectId = myPlayer->GetId();
				info.position = Converter::MakeVector(myPlayer->GetPosition());
				spawn.players.push_back(info);
			}
			else
			{
				myPlayer->SetPosition(Vector2DI(gameMap->GetMap()[0].size() - packet.position.x, packet.position.y));
				info.position = Converter::MakeVector(myPlayer->GetPosition());
				spawn.players.push_back(info);
			}
			session->Send(&spawn);
		}

		// send other player list
		{
			gen::mmo::Spawn spawn;
			spawn.isMine = false;
			for (const auto& player : gameMap->Players())
			{
				if (player->GetId() != myPlayer->GetId())
				{
					gen::mmo::PlayerInfo info;
					info.objectId = player->GetId();
					info.position = Converter::MakeVector(player->GetPosition());
					//info.name = player->GetName();
					spawn.players.push_back(info);
				}
			}
			session->Send(&spawn);
		}

		// notify exist players
		{
			gen::mmo::Spawn spawn;
			gen::mmo::PlayerInfo info;
			spawn.isMine = false;
			info.objectId = myPlayer->GetId();
			info.position = Converter::MakeVector(myPlayer->GetPosition());
			spawn.players.push_back(info);
			gameMap->Broadcast(&spawn, myPlayer->GetId());
		}
	}
}

	