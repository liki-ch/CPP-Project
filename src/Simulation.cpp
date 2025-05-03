#include "../include/Simulation.h"
#include "../include/Config.h"
#include <algorithm>
#include <random>
#include <thread>
#include <iostream> 

Simulation::Simulation(const Config& config)
    : fieldSize(config.field_size),
      timeStep(config.time_step),
      containmentField(std::make_unique<ContainmentField>(config)),
      threadManager(std::make_unique<ThreadManager>(config.initial_threads)),
      numThreads(config.initial_threads) {
    initializeParticles(config);
}

Simulation::~Simulation() {
    stop();
}

void Simulation::initializeParticles(const Config& config) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-fieldSize/2, fieldSize/2);
    std::uniform_real_distribution<> vel_dis(-1.0, 1.0); // Velocity range
    
    for (size_t i = 0; i < config.num_particles; ++i) {
        auto particle = std::make_unique<Particle>(
            dis(gen), dis(gen),
            config.initial_energy,
            config.particle_radius,
            config.max_energy
        );
        particle->setVelocity(vel_dis(gen), vel_dis(gen));
        particles.push_back(std::move(particle));
    }
    std::cout << "Initialized " << particles.size() << " particles." << std::endl;
}

void Simulation::setContainmentField(std::unique_ptr<ContainmentField> field) {
    containmentField = std::move(field);
}

void Simulation::start() {
    running = true;
    // Remove creation of worker threads - let ThreadManager handle it
    threadManager->start();
    std::cout << "Simulation started with " << numThreads << " threads." << std::endl;
}

void Simulation::stop() {
    running = false;
    threadManager->stop();
    // Keep this part to ensure proper cleanup
    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    workerThreads.clear();
    std::cout << "Simulation stopped." << std::endl;
}

void Simulation::step() {
    removeEscapedParticles();
    
    // Start the thread manager if it's not already running
    if (!threadManager->isRunning()) {
        threadManager->start();
    }
    
    // Divide particles among threads for force application
    size_t particlesPerThread = std::max(size_t(1), particles.size() / numThreads);
    for (size_t i = 0; i < particles.size(); i += particlesPerThread) {
        size_t end = std::min(i + particlesPerThread, particles.size());
        threadManager->addTask([this, i, end]() {
            for (size_t j = i; j < end; j++) {
                // Apply forces to each particle in the range
                double x = particles[j]->getX();
                double y = particles[j]->getY();
                double distance = std::sqrt(x*x + y*y);
                
                if (distance > 0) {
                    double force = containmentField->getContainmentForce(*particles[j]);
                    double dirX = -x / distance;
                    double dirY = -y / distance;
                    double vx = particles[j]->getVX() + force * dirX * timeStep;  
                    double vy = particles[j]->getVY() + force * dirY * timeStep;
                }
            }
        });
    }
    
    // Wait for all force calculation tasks to complete
    threadManager->waitForCompletion();
    
    // Update positions (could also be parallelized)
    updatePositions(timeStep);
    handleCollisions();
    containmentField->update(timeStep);
}

void Simulation::addParticle(std::unique_ptr<Particle> particle) {
    if (particle) {
        particles.push_back(std::move(particle));
    }
}

void Simulation::removeEscapedParticles() {
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [this](const std::unique_ptr<Particle>& p) {
                return !containmentField->isParticleContained(*p);
            }),
        particles.end()
    );
}

size_t Simulation::getParticleCount() const {
    return particles.size();
}

const std::vector<std::unique_ptr<Particle>>& Simulation::getParticles() const {
    return particles;
}

double Simulation::getTotalEnergy() const {
    double total = 0.0;
    for (const auto& particle : particles) {
        total += particle->getEnergy();
    }
    return total;
}

void Simulation::setNumThreads(size_t newNumThreads) {
    numThreads = newNumThreads;
    threadManager->setNumThreads(newNumThreads);
}

size_t Simulation::getNumThreads() const {
    return numThreads;
}

void Simulation::updatePositions(double dt) {
    // Divide particles among threads for position updates
    size_t particlesPerThread = std::max(size_t(1), particles.size() / numThreads);
    for (size_t i = 0; i < particles.size(); i += particlesPerThread) {
        size_t end = std::min(i + particlesPerThread, particles.size());
        threadManager->addTask([this, i, end, dt]() {
            for (size_t j = i; j < end; j++) {
                double x = particles[j]->getX() + particles[j]->getVX() * dt;
                double y = particles[j]->getY() + particles[j]->getVY() * dt;
                particles[j]->setPosition(x, y);
            }
        });
    }
    
    // Wait for all position updates to complete
    threadManager->waitForCompletion();
}

void Simulation::handleCollisions() {
    for (size_t i = 0; i < particles.size(); i++) {
        for (size_t j = i + 1; j < particles.size(); j++) {
            if (particles[i]->isColliding(*particles[j])) {
                particles[i]->collide(*particles[j]);
            }
        }
    }
}