#ifndef STAGES_UNPACKING_CUSTOM_MIDAS_UNPACKER_STAGE_H
#define STAGES_UNPACKING_CUSTOM_MIDAS_UNPACKER_STAGE_H

#include <memory>
#include <string>
#include <unordered_map>

#include <TObject.h>
#include <TList.h>
#include <nlohmann/json.hpp>

#include "stages/unpacking/base_midas_unpacker_stage.h"
#include "common/unpacking/EventUnpacker.hh"

class CustomMidasUnpackerStage : public BaseMidasUnpackerStage {
public:
    CustomMidasUnpackerStage();
    ~CustomMidasUnpackerStage() override;

    void ProcessMidasEvent(TMEvent& event) override;
    std::string Name() const override;

protected:
    void OnInit() override;

private:
    std::unique_ptr<unpackers::EventUnpacker> unpacker_;

    std::string unpackerClassName_;  ///< Store unpacker class name for logging

    ClassDefOverride(CustomMidasUnpackerStage, 1);  // Use ClassDefOverride for ROOT compatibility
};

#endif // STAGES_UNPACKING_CUSTOM_MIDAS_UNPACKER_STAGE_H