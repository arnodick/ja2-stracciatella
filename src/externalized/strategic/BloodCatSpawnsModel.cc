#include "BloodCatSpawnsModel.h"
#include "GameSettings.h"
#include "JsonUtility.h"

BloodCatSpawnsModel::BloodCatSpawnsModel(uint8_t sectorId_,
	bool isLair_, bool isArena_,
	int8_t bloodCatsSpawnsEasy_, int8_t bloodCatsSpawnsMedium_, int8_t bloodCatsSpawnsHard_)
		:sectorId(sectorId_),
			isLair(isLair_), isArena(isArena_),
			bloodCatsSpawnsEasy(bloodCatsSpawnsEasy_), bloodCatsSpawnsMedium(bloodCatsSpawnsMedium_), bloodCatsSpawnsHard(bloodCatsSpawnsHard_) {}

int8_t BloodCatSpawnsModel::getSpawnsByDifficulty(uint8_t const difficultyLevel) const
{
	switch (difficultyLevel) {
		case DIF_LEVEL_EASY:
			return bloodCatsSpawnsEasy;
		case DIF_LEVEL_MEDIUM:
			return bloodCatsSpawnsMedium;
		case DIF_LEVEL_HARD:
			return bloodCatsSpawnsHard;
		default:
			throw std::runtime_error(ST::format("Unsupported difficulty level: {}", difficultyLevel).to_std_string());
	}
}

BloodCatSpawnsModel* BloodCatSpawnsModel::deserialize(const JsonValue& json)
{
	auto obj = json.toObject();
	uint8_t sectorId = JsonUtility::parseSectorID(obj["sector"]);
	return new BloodCatSpawnsModel(
		sectorId,
		obj.GetBool("isLair"),
		obj.GetBool("isArena"),
		obj.GetInt("bloodCatsSpawnsEasy"),
		obj.GetInt("bloodCatsSpawnsMedium"),
		obj.GetInt("bloodCatsSpawnsHard")
		);
}

