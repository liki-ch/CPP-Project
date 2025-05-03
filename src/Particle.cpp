#include "../include/Particle.h"
#include <cmath>
#include <thread>
#include <chrono>
#include <algorithm>

Particle::Particle(double x, double y, double energy, double radius, double max_energy)
    : x(x), y(y), vx(0.0), vy(0.0), energy(energy), MAX_ENERGY(max_energy), PARTICLE_RADIUS(radius) {
    // Fix: don't override energy
    // this->energy = -100.0;
}

Particle::~Particle() {
}

double Particle::getX() const {
    // Fix: return actual x without scaling
    return x;
}

double Particle::getY() const {
    // Fix: return actual y without scaling
    return y;
}

void Particle::setPosition(double newX, double newY) {
    // Fix: don't scale position
    x = newX;
    y = newY;
}

double Particle::getVX() const {
    // Fix: return actual velocity
    return vx;
}

double Particle::getVY() const {
    // Fix: return actual velocity
    return vy;
}

void Particle::setVelocity(double newVX, double newVY) {
    std::lock_guard<std::mutex> lock(particleMutex);
    vx = newVX;
    vy = newVY;
}

double Particle::getEnergy() const {
    // Fix: return actual energy
    return energy;
}

double Particle::getMaxEnergy() const {
    // Fix: return actual MAX_ENERGY
    return MAX_ENERGY;
}

void Particle::setEnergy(double newEnergy) {
    // Fix: don't scale energy, but do clamp to MAX_ENERGY
    energy = std::min(newEnergy, MAX_ENERGY);
}

void Particle::addEnergy(double delta) {
    // Implement energy addition with maximum limit
    energy = std::min(energy + delta, MAX_ENERGY);
}

void Particle::collide(Particle& other) {
    // Implement proper collision physics
    // Exchange velocities (simplified elastic collision)
    double tempVX = vx;
    double tempVY = vy;
    
    vx = other.vx;
    vy = other.vy;
    
    other.vx = tempVX;
    other.vy = tempVY;
    
    // Energy transfer
    double energyTransfer = energy * 0.1;
    energy -= energyTransfer;
    other.addEnergy(energyTransfer);
}

bool Particle::isColliding(const Particle& other) const {
    // Fix: implement collision detection
    double dx = x - other.x;
    double dy = y - other.y;
    double distance = std::sqrt(dx*dx + dy*dy);
    return distance < (PARTICLE_RADIUS + other.PARTICLE_RADIUS);
}
