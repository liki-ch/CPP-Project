#include "../include/ContainmentField.h"
#include "../include/Particle.h"
#include "../include/Config.h"
#include <cmath>
#include <algorithm>

ContainmentField::ContainmentField(const Config& config)
    : size(config.field_size), fieldStrength(config.initial_strength), decayRate(config.initial_decay_rate), GRID_SIZE(config.field_grid_size), fieldEnergy(0.0) {
    initializeField();
}

ContainmentField::~ContainmentField() {
    // Clean up any dynamically allocated energy pulses
    for (auto* pulse : energyPulses) {
        delete pulse;
    }
    energyPulses.clear();
}

void ContainmentField::initializeField() {
    fieldData.resize(GRID_SIZE * GRID_SIZE, 0.0); 
    // Initialize the field energy based on the sum of all grid values
    fieldEnergy = 0.0;
}

double ContainmentField::getContainmentForce(const Particle& particle) const {
    double x = particle.getX();
    double y = particle.getY();
    
    double distance = std::sqrt(x*x + y*y);
    if (distance < 1e-10) {
        return 0.0; // No force at center
    }
    
    // Scale force based on distance from center toward edge
    double normalized_distance = distance / (size / 2.0);
    return fieldStrength * normalized_distance;
}

bool ContainmentField::isParticleContained(const Particle& particle) const {
    double x = particle.getX();
    double y = particle.getY();
    
    // Check if particle is within half the field size from center
    double distanceFromCenter = std::sqrt(x*x + y*y);
    return distanceFromCenter < (size / 2.0);
}

void ContainmentField::update(double dt) {
    std::lock_guard<std::mutex> lock(fieldMutex);
    fieldEnergy = 0.0; // Reset field energy before recalculating
    
    for (size_t i = 0; i < fieldData.size(); ++i) {
        fieldData[i] *= (1.0 - decayRate * dt);
        fieldEnergy += fieldData[i]; // Accumulate total field energy
    }
    
    // Update energy pulses if any exist
    auto it = energyPulses.begin();
    while (it != energyPulses.end()) {
        (*it)->lifetime -= dt;
        if ((*it)->lifetime <= 0.0) {
            delete *it;
            it = energyPulses.erase(it);
        } else {
            // Add pulse energy to field at the appropriate grid position
            int gridX = static_cast<int>(((*it)->x + size/2) * GRID_SIZE / size);
            int gridY = static_cast<int>(((*it)->y + size/2) * GRID_SIZE / size);
            
            if (gridX >= 0 && gridX < static_cast<int>(GRID_SIZE) && 
                gridY >= 0 && gridY < static_cast<int>(GRID_SIZE)) {
                size_t index = gridY * GRID_SIZE + gridX;
                fieldData[index] += (*it)->strength * dt;
                fieldEnergy += (*it)->strength * dt;
            }
            ++it;
        }
    }
}

void ContainmentField::setFieldStrength(double strength) {
    std::lock_guard<std::mutex> lock(fieldMutex);
    fieldStrength = strength;
}

double ContainmentField::getFieldStrength() const {
    std::lock_guard<std::mutex> lock(fieldMutex);
    return fieldStrength;
}

void ContainmentField::setDecayRate(double rate) {
    std::lock_guard<std::mutex> lock(fieldMutex);
    decayRate = rate;
}

double ContainmentField::getDecayRate() const {
    std::lock_guard<std::mutex> lock(fieldMutex);
    return decayRate;
}

double ContainmentField::getSize() const {
    return size;
}

double ContainmentField::getFieldEnergy() const {
    std::lock_guard<std::mutex> lock(fieldMutex);
    return fieldEnergy;
}