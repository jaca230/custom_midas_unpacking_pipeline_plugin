#include "stages/unpacking/custom_midas_unpacker_stage.h"

#include <TClass.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

#include <TROOT.h>
#include <TClassTable.h>
#include <iostream>
#include <string>

ClassImp(CustomMidasUnpackerStage)

CustomMidasUnpackerStage::CustomMidasUnpackerStage() = default;

CustomMidasUnpackerStage::~CustomMidasUnpackerStage() = default;

void CustomMidasUnpackerStage::OnInit() {
    spdlog::debug("[CustomMidasUnpackerStage::OnInit] Starting initialization");

    try {
        unpackerClassName_ = parameters_.at("unpacker_class").get<std::string>();
        spdlog::debug("[CustomMidasUnpackerStage::OnInit] Unpacker class from config: '{}'", unpackerClassName_);
    } catch (const std::exception& e) {
        spdlog::error("[CustomMidasUnpackerStage::OnInit] Missing or invalid 'unpacker_class' config: {}", e.what());
        throw;
    }

    TClass* cls = TClass::GetClass(unpackerClassName_.c_str());
    if (!cls) {
        spdlog::error("[CustomMidasUnpackerStage::OnInit] Unknown unpacker class '{}'", unpackerClassName_);
        throw std::runtime_error("Unknown unpacker class: " + unpackerClassName_);
    }

    spdlog::debug("[CustomMidasUnpackerStage::OnInit] Creating instance of '{}'", unpackerClassName_);
    TObject* obj = static_cast<TObject*>(cls->New());
    if (!obj) {
        spdlog::error("[CustomMidasUnpackerStage::OnInit] Failed to instantiate unpacker '{}'", unpackerClassName_);
        throw std::runtime_error("Failed to instantiate: " + unpackerClassName_);
    }

    unpacker_.reset(dynamic_cast<unpackers::EventUnpacker*>(obj));
    if (!unpacker_) {
        delete obj;
        spdlog::error("[CustomMidasUnpackerStage::OnInit] '{}' is not derived from EventUnpacker", unpackerClassName_);
        throw std::runtime_error("Unpacker is not derived from EventUnpacker");
    }

    spdlog::debug("[CustomMidasUnpackerStage::OnInit] Successfully instantiated unpacker '{}'", unpackerClassName_);
}

void CustomMidasUnpackerStage::ProcessMidasEvent(TMEvent& event) {
    spdlog::debug("[CustomMidasUnpackerStage::ProcessMidasEvent] Processing event");

    if (!unpacker_) {
        spdlog::error("[CustomMidasUnpackerStage] No unpacker instantiated; skipping event");
        return;
    }

    event.FindAllBanks();
    spdlog::debug("[CustomMidasUnpackerStage::ProcessMidasEvent] Called FindAllBanks() on event");

    unpacker_->UnpackEvent(&event);
    spdlog::debug("[CustomMidasUnpackerStage::ProcessMidasEvent] Called UnpackEvent() using unpacker '{}'", unpackerClassName_);

    const auto& collections = unpacker_->GetCollections();
    spdlog::debug("[CustomMidasUnpackerStage::ProcessMidasEvent] Unpacker returned {} collections", collections.size());

    std::unordered_map<std::string, std::unique_ptr<PipelineDataProduct>> productsToAdd;

    for (const auto& [label, vecPtr] : collections) {
        if (!vecPtr) {
            spdlog::debug("[CustomMidasUnpackerStage::ProcessMidasEvent] Skipping null vector for label '{}'", label);
            continue;
        }
        if (vecPtr->empty()) {
            spdlog::debug("[CustomMidasUnpackerStage::ProcessMidasEvent] Skipping empty vector for label '{}'", label);
            continue;
        }

        auto list = std::make_unique<TList>();
        list->SetOwner(kTRUE);

        int addedCount = 0;
        for (const auto& shared_dp : *vecPtr) {
            if (!shared_dp) continue;
            TObject* clone = shared_dp->Clone();
            if (!clone) continue;
            list->Add(clone);
            ++addedCount;
        }

        if (list->IsEmpty()) {
            spdlog::debug("[CustomMidasUnpackerStage::ProcessMidasEvent] No valid objects cloned for label '{}'", label);
            continue;
        }

        auto pdp = std::make_unique<PipelineDataProduct>();
        pdp->setName(label);
        pdp->setObject(std::move(list));
        pdp->addTag("unpacked_data");
        pdp->addTag("built_by_custom_midas_unpacker");

        productsToAdd.emplace(label, std::move(pdp));

        spdlog::debug("[CustomMidasUnpackerStage::ProcessMidasEvent] Prepared data product '{}' with {} objects", label, addedCount);
    }

    if (!productsToAdd.empty()) {
        std::vector<std::pair<std::string, std::unique_ptr<PipelineDataProduct>>> productsVec;
        productsVec.reserve(productsToAdd.size());
        for (auto& p : productsToAdd) {
            productsVec.emplace_back(std::move(p));
        }
        getDataProductManager()->addOrUpdateMultiple(std::move(productsVec));
    }
}


std::string CustomMidasUnpackerStage::Name() const {
    return "CustomMidasUnpackerStage";
}