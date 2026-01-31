#pragma once

namespace TheEngine {
    namespace physics {
        namespace detail {
            constexpr float kSeparation = 0.01f;

            inline void applyFriction(Vec2& velocity, const Vec2& normal, float friction) {
                float clamped = TheEngine::clamp(friction, 0.0f, 1.0f);
                Vec2 tangential = velocity - normal * velocity.dot(normal);
                velocity -= tangential * clamped;
            }

            inline bool tryPushDynamicChain(std::vector<AABB>& dynamicBoxes,
                                            std::vector<Vec2>& dynamicVelocities,
                                            const std::vector<float>& dynamicMasses,
                                            const std::vector<AABB>& staticBoxes,
                                            size_t hitIndex,
                                            const Vec2& pushDelta,
                                            const Vec2& pushDir,
                                            float pushSpeed,
                                            float maxMass) {
                if (pushDelta.x == 0.0f && pushDelta.y == 0.0f) return false;
                if (pushSpeed <= 0.0f) return false;

                std::vector<size_t> chain;
                chain.push_back(hitIndex);

                bool expanded = true;
                while (expanded) {
                    expanded = false;
                    for (size_t i = 0; i < chain.size(); ++i) {
                        const size_t idx = chain[i];
                        AABB moved = dynamicBoxes[idx];
                        moved.pos += pushDelta;

                        for (size_t j = 0; j < dynamicBoxes.size(); ++j) {
                            if (j == idx) continue;
                            bool already = false;
                            for (size_t k = 0; k < chain.size(); ++k) {
                                if (chain[k] == j) {
                                    already = true;
                                    break;
                                }
                            }
                            if (already) continue;

                            if (moved.intersectAABB(dynamicBoxes[j])) {
                                chain.push_back(j);
                                expanded = true;
                            }
                        }
                    }
                }

                float totalMass = 0.0f;
                for (size_t i = 0; i < chain.size(); ++i) {
                    size_t idx = chain[i];
                    totalMass += (idx < dynamicMasses.size()) ? dynamicMasses[idx] : 1.0f;
                }
                if (maxMass >= 0.0f && totalMass > maxMass) return false;

                for (size_t i = 0; i < chain.size(); ++i) {
                    AABB moved = dynamicBoxes[chain[i]];
                    moved.pos += pushDelta;
                    for (const auto& staticBox : staticBoxes) {
                        if (moved.intersectAABB(staticBox)) {
                            return false;
                        }
                    }
                }

                for (size_t i = 0; i < chain.size(); ++i) {
                    size_t idx = chain[i];
                    dynamicBoxes[idx].pos += pushDelta;
                    dynamicVelocities[idx] += pushDir * pushSpeed;
                }
                return true;
            }
        }

        inline void PhysicsSystem::stepWorld(WorldStepConfig& config, float dt) {
            using detail::applyFriction;
            using detail::kSeparation;
            using detail::tryPushDynamicChain;

            // Player vs static/dynamic
            if (config.playerVelocity.x != 0.0f || config.playerVelocity.y != 0.0f) {
                Vec2 delta = config.playerVelocity * dt;

                Sweep staticSweep = config.playerBox.sweepInto(config.staticBoxes, delta);

                Sweep dynamicSweep;
                dynamicSweep.time = 1.0f;
                dynamicSweep.pos = config.playerBox.pos + delta;
                size_t dynamicHitIndex = config.dynamicBoxes.size();

                for (size_t i = 0; i < config.dynamicBoxes.size(); ++i) {
                    Sweep sweep = config.dynamicBoxes[i].sweepAABB(config.playerBox, delta);
                    if (sweep.time < dynamicSweep.time) {
                        dynamicSweep = std::move(sweep);
                        dynamicHitIndex = i;
                    }
                }

                bool hitDynamic = dynamicHitIndex < config.dynamicBoxes.size() && dynamicSweep.time < staticSweep.time;
                const Sweep& sweep = hitDynamic ? dynamicSweep : staticSweep;

                config.playerBox.pos = sweep.pos;

                if (sweep.hit) {
                    Vec2 normal = sweep.hit->normal;
                    float vn = config.playerVelocity.dot(normal);
                    if (vn < 0.0f) {
                        config.playerVelocity -= normal * vn;
                        if (hitDynamic) {
                            config.dynamicVelocities[dynamicHitIndex] += (-normal) * (-vn);
                        }
                    }

                    Vec2 remaining = delta * (1.0f - sweep.time);
                    if (hitDynamic) {
                        Vec2 pushDir = -normal;
                        float pushAmount = remaining.dot(pushDir);
                        if (pushAmount > 0.0f) {
                            Vec2 pushDelta = pushDir * pushAmount;
                            float pushSpeed = config.playerVelocity.dot(pushDir);
                            if (tryPushDynamicChain(config.dynamicBoxes, config.dynamicVelocities, config.dynamicMasses,
                                                    config.staticBoxes, dynamicHitIndex, pushDelta, pushDir,
                                                    pushSpeed, config.playerPushMass)) {
                                config.playerBox.pos += pushDelta;
                                remaining -= pushDelta;
                            }
                        }
                    }

                    float playerFriction = hitDynamic
                        ? (config.playerFrictionDynamic + config.dynamicFrictions[dynamicHitIndex]) * 0.5f
                        : config.playerFrictionStatic;
                    applyFriction(config.playerVelocity, normal, playerFriction);

                    config.playerBox.pos += normal * kSeparation;

                    float remainingIntoNormal = remaining.dot(normal);
                    if (remainingIntoNormal < 0.0f) {
                        remaining -= normal * remainingIntoNormal;
                    }

                    if (remaining.x != 0.0f || remaining.y != 0.0f) {
                        Sweep staticSlide = config.playerBox.sweepInto(config.staticBoxes, remaining);
                        Sweep dynamicSlide;
                        dynamicSlide.time = 1.0f;
                        dynamicSlide.pos = config.playerBox.pos + remaining;
                        size_t dynamicSlideIndex = config.dynamicBoxes.size();

                        for (size_t i = 0; i < config.dynamicBoxes.size(); ++i) {
                            Sweep slide = config.dynamicBoxes[i].sweepAABB(config.playerBox, remaining);
                            if (slide.time < dynamicSlide.time) {
                                dynamicSlide = std::move(slide);
                                dynamicSlideIndex = i;
                            }
                        }

                        bool slideHitsDynamic = dynamicSlideIndex < config.dynamicBoxes.size()
                            && dynamicSlide.time < staticSlide.time;
                        const Sweep& slideSweep = slideHitsDynamic ? dynamicSlide : staticSlide;
                        config.playerBox.pos = slideSweep.pos;

                        if (slideSweep.hit) {
                            config.playerBox.pos += slideSweep.hit->normal * kSeparation;
                            if (slideHitsDynamic) {
                                float slideVn = config.playerVelocity.dot(slideSweep.hit->normal);
                                if (slideVn < 0.0f) {
                                    config.dynamicVelocities[dynamicSlideIndex] += (-slideSweep.hit->normal) * (-slideVn);
                                }
                                float slideFriction = (config.playerFrictionDynamic
                                    + config.dynamicFrictions[dynamicSlideIndex]) * 0.5f;
                                applyFriction(config.playerVelocity, slideSweep.hit->normal, slideFriction);
                            } else {
                                applyFriction(config.playerVelocity, slideSweep.hit->normal, config.playerFrictionStatic);
                            }
                        }
                    }
                }
            }

            // Dynamic AABBs with gravity and collisions
            for (size_t i = 0; i < config.dynamicBoxes.size(); ++i) {
                config.dynamicVelocities[i].y -= config.gravity * dt;
                Vec2 delta = config.dynamicVelocities[i] * dt;

                std::vector<AABB> allColliders = config.staticBoxes;
                allColliders.push_back(config.playerBox);
                for (size_t j = 0; j < config.dynamicBoxes.size(); ++j) {
                    if (i != j) allColliders.push_back(config.dynamicBoxes[j]);
                }

                Sweep sweep = config.dynamicBoxes[i].sweepInto(allColliders, delta);
                config.dynamicBoxes[i].pos = sweep.pos;

                if (sweep.hit) {
                    Vec2 normal = sweep.hit->normal;
                    float vn = config.dynamicVelocities[i].dot(normal);

                    int hitDynamicIndex = -1;
                    if (!allColliders.empty() && sweep.hit->collider) {
                        const auto* base = allColliders.data();
                        size_t hitIndex = static_cast<size_t>(sweep.hit->collider - base);
                        if (hitIndex > config.staticBoxes.size()) {
                            size_t dynamicHit = hitIndex - (config.staticBoxes.size() + 1);
                            if (dynamicHit < config.dynamicBoxes.size() && dynamicHit != i) {
                                hitDynamicIndex = static_cast<int>(dynamicHit);
                            }
                        }
                    }

                    float collisionFriction = config.staticFriction;
                    float collisionRestitution = config.staticRestitution;
                    if (!allColliders.empty() && sweep.hit->collider) {
                        const auto* base = allColliders.data();
                        size_t hitIndex = static_cast<size_t>(sweep.hit->collider - base);
                        if (hitIndex < config.staticBoxes.size()) {
                            collisionFriction = config.staticFriction;
                            collisionRestitution = config.staticRestitution;
                        } else if (hitIndex == config.staticBoxes.size()) {
                            collisionFriction = config.playerFrictionDynamic;
                            collisionRestitution = config.playerRestitution;
                        } else {
                            size_t dynamicHit = hitIndex - (config.staticBoxes.size() + 1);
                            if (dynamicHit < config.dynamicFrictions.size()) {
                                collisionFriction = config.dynamicFrictions[dynamicHit];
                            }
                            if (dynamicHit < config.dynamicRestitutions.size()) {
                                collisionRestitution = config.dynamicRestitutions[dynamicHit];
                            }
                        }
                    }
                    float selfFriction = (i < config.dynamicFrictions.size())
                        ? config.dynamicFrictions[i]
                        : config.staticFriction;
                    float selfRestitution = (i < config.dynamicRestitutions.size())
                        ? config.dynamicRestitutions[i]
                        : config.staticRestitution;
                    float combinedRestitution = (collisionRestitution + selfRestitution) * 0.5f;
                    if (vn < 0.0f) {
                        config.dynamicVelocities[i] -= normal * ((1.0f + combinedRestitution) * vn);
                    }
                    applyFriction(config.dynamicVelocities[i], normal, (collisionFriction + selfFriction) * 0.5f);

                    config.dynamicBoxes[i].pos += normal * kSeparation;

                    Vec2 remaining = delta * (1.0f - sweep.time);
                    if (hitDynamicIndex >= 0) {
                        Vec2 pushDir = -normal;
                        float pushAmount = remaining.dot(pushDir);
                        if (pushAmount > 0.0f) {
                            Vec2 pushDelta = pushDir * pushAmount;
                            float pushSpeed = config.dynamicVelocities[i].dot(pushDir);
                            float maxMass = (i < config.dynamicMasses.size()) ? config.dynamicMasses[i] : 1.0f;
                            if (tryPushDynamicChain(config.dynamicBoxes, config.dynamicVelocities,
                                                    config.dynamicMasses, config.staticBoxes,
                                                    static_cast<size_t>(hitDynamicIndex), pushDelta, pushDir,
                                                    pushSpeed, maxMass)) {
                                remaining -= pushDelta;
                            }
                        }
                    }

                    float remainingIntoNormal = remaining.dot(normal);
                    if (remainingIntoNormal < 0.0f) {
                        remaining -= normal * remainingIntoNormal;
                    }

                    if (remaining.x != 0.0f || remaining.y != 0.0f) {
                        Sweep slideSweep = config.dynamicBoxes[i].sweepInto(allColliders, remaining);
                        config.dynamicBoxes[i].pos = slideSweep.pos;
                        if (slideSweep.hit) {
                            config.dynamicBoxes[i].pos += slideSweep.hit->normal * kSeparation;
                            applyFriction(config.dynamicVelocities[i], slideSweep.hit->normal,
                                          (collisionFriction + selfFriction) * 0.5f);
                        }
                    }
                }
            }

            // Circles with gravity and static/player collisions
            for (auto& circle : config.circles) {
                circle.velocity.y -= config.gravity * dt;
                circle.pos += circle.velocity * dt;

                for (const auto& box : config.staticBoxes) {
                    auto hit = box.intersectCircle(circle.pos, circle.radius);
                    if (hit) {
                        circle.pos += hit->delta;
                        float vn = circle.velocity.dot(hit->normal);
                        if (vn < 0.0f) {
                            float restitution = (circle.restitution + config.staticRestitution) * 0.5f;
                            circle.velocity -= hit->normal * ((1.0f + restitution) * vn);
                        }
                        applyFriction(circle.velocity, hit->normal,
                                      (circle.friction + config.staticFriction) * 0.5f);
                    }
                }

                auto playerHit = config.playerBox.intersectCircle(circle.pos, circle.radius);
                if (playerHit) {
                    circle.pos += playerHit->delta;
                    float vn = circle.velocity.dot(playerHit->normal);
                    if (vn < 0.0f) {
                        float restitution = (circle.restitution + config.playerRestitution) * 0.5f;
                        circle.velocity -= playerHit->normal * ((1.0f + restitution) * vn);
                    }
                    applyFriction(circle.velocity, playerHit->normal,
                                  (circle.friction + config.playerFrictionDynamic) * 0.5f);
                }
            }

            // Circle-circle collisions
            for (size_t i = 0; i < config.circles.size(); ++i) {
                for (size_t j = i + 1; j < config.circles.size(); ++j) {
                    auto& a = config.circles[i];
                    auto& b = config.circles[j];
                    Vec2 delta = b.pos - a.pos;
                    float distSq = delta.x * delta.x + delta.y * delta.y;
                    float minDist = a.radius + b.radius;
                    if (distSq >= minDist * minDist) continue;

                    Vec2 normal(1.0f, 0.0f);
                    float dist = std::sqrt(distSq);
                    if (dist > EPSILON) normal = delta / dist;

                    float overlap = minDist - dist;
                    float massA = (a.mass > 0.0f) ? a.mass : 1.0f;
                    float massB = (b.mass > 0.0f) ? b.mass : 1.0f;
                    float invMassA = 1.0f / massA;
                    float invMassB = 1.0f / massB;
                    float invMassSum = invMassA + invMassB;

                    a.pos -= normal * (overlap * (invMassA / invMassSum));
                    b.pos += normal * (overlap * (invMassB / invMassSum));

                    float relVel = (b.velocity - a.velocity).dot(normal);
                    if (relVel < 0.0f) {
                        float combinedRest = (a.restitution + b.restitution) * 0.5f;
                        float impulse = -(1.0f + combinedRest) * relVel / invMassSum;
                        Vec2 impulseVec = normal * impulse;
                        a.velocity -= impulseVec * invMassA;
                        b.velocity += impulseVec * invMassB;
                    }

                    float combinedFriction = (a.friction + b.friction) * 0.5f;
                    applyFriction(a.velocity, normal, combinedFriction);
                    applyFriction(b.velocity, -normal, combinedFriction);
                }
            }

            // Dynamic AABB vs Dynamic AABB overlap resolution
            for (size_t i = 0; i < config.dynamicBoxes.size(); ++i) {
                for (size_t j = i + 1; j < config.dynamicBoxes.size(); ++j) {
                    auto hit = config.dynamicBoxes[i].intersectAABB(config.dynamicBoxes[j]);
                    if (!hit) continue;

                    Vec2 separation = hit->delta;
                    Vec2 normal = hit->normal;

                    float massA = (i < config.dynamicMasses.size() && config.dynamicMasses[i] > 0.0f)
                        ? config.dynamicMasses[i]
                        : 1.0f;
                    float massB = (j < config.dynamicMasses.size() && config.dynamicMasses[j] > 0.0f)
                        ? config.dynamicMasses[j]
                        : 1.0f;
                    float invMassA = 1.0f / massA;
                    float invMassB = 1.0f / massB;
                    float invMassSum = invMassA + invMassB;

                    Vec2 corrA = separation * (invMassA / invMassSum);
                    Vec2 corrB = separation * (invMassB / invMassSum);
                    Vec2 proposedA = config.dynamicBoxes[i].pos - corrA;
                    Vec2 proposedB = config.dynamicBoxes[j].pos + corrB;

                    bool blockedA = false;
                    bool blockedB = false;
                    for (const auto& staticBox : config.staticBoxes) {
                        AABB movedA = config.dynamicBoxes[i];
                        movedA.pos = proposedA;
                        if (!blockedA && movedA.intersectAABB(staticBox)) blockedA = true;
                        AABB movedB = config.dynamicBoxes[j];
                        movedB.pos = proposedB;
                        if (!blockedB && movedB.intersectAABB(staticBox)) blockedB = true;
                        if (blockedA && blockedB) break;
                    }

                    if (blockedA && blockedB) {
                        // keep positions
                    } else if (blockedA) {
                        config.dynamicBoxes[j].pos += separation;
                    } else if (blockedB) {
                        config.dynamicBoxes[i].pos -= separation;
                    } else {
                        config.dynamicBoxes[i].pos = proposedA;
                        config.dynamicBoxes[j].pos = proposedB;
                    }

                    float relVel = (config.dynamicVelocities[j] - config.dynamicVelocities[i]).dot(normal);
                    if (relVel < 0.0f) {
                        float restA = (i < config.dynamicRestitutions.size())
                            ? config.dynamicRestitutions[i]
                            : config.staticRestitution;
                        float restB = (j < config.dynamicRestitutions.size())
                            ? config.dynamicRestitutions[j]
                            : config.staticRestitution;
                        float combinedRest = (restA + restB) * 0.5f;
                        float impulse = -(1.0f + combinedRest) * relVel / invMassSum;
                        Vec2 impulseVec = normal * impulse;
                        config.dynamicVelocities[i] -= impulseVec * invMassA;
                        config.dynamicVelocities[j] += impulseVec * invMassB;
                    }

                    float frictionA = (i < config.dynamicFrictions.size())
                        ? config.dynamicFrictions[i]
                        : config.staticFriction;
                    float frictionB = (j < config.dynamicFrictions.size())
                        ? config.dynamicFrictions[j]
                        : config.staticFriction;
                    float combinedFriction = (frictionA + frictionB) * 0.5f;
                    applyFriction(config.dynamicVelocities[i], normal, combinedFriction);
                    applyFriction(config.dynamicVelocities[j], -normal, combinedFriction);
                }
            }

            // Circle vs dynamic AABB collisions (symmetric)
            for (size_t c = 0; c < config.circles.size(); ++c) {
                auto& circle = config.circles[c];
                float invMassCircle = 1.0f / ((circle.mass > 0.0f) ? circle.mass : 1.0f);
                for (size_t i = 0; i < config.dynamicBoxes.size(); ++i) {
                    auto hit = config.dynamicBoxes[i].intersectCircle(circle.pos, circle.radius);
                    if (!hit) continue;

                    float invMassBox = 1.0f / ((i < config.dynamicMasses.size()
                        && config.dynamicMasses[i] > 0.0f) ? config.dynamicMasses[i] : 1.0f);
                    float invMassSum = invMassCircle + invMassBox;

                    Vec2 normal = hit->normal;
                    Vec2 delta = hit->delta;

                    Vec2 circleCorrection = delta * (invMassCircle / invMassSum);
                    Vec2 boxCorrection = delta * (invMassBox / invMassSum);
                    Vec2 proposedBoxPos = config.dynamicBoxes[i].pos - boxCorrection;
                    bool boxBlocked = false;
                    for (const auto& staticBox : config.staticBoxes) {
                        AABB moved = config.dynamicBoxes[i];
                        moved.pos = proposedBoxPos;
                        if (moved.intersectAABB(staticBox)) {
                            boxBlocked = true;
                            break;
                        }
                    }
                    if (boxBlocked) {
                        circle.pos += delta;
                    } else {
                        circle.pos += circleCorrection;
                        config.dynamicBoxes[i].pos = proposedBoxPos;
                    }

                    Vec2 pushDir = -normal;
                    float pushSpeed = circle.velocity.dot(pushDir);
                    if (pushSpeed > 0.0f) {
                        Vec2 pushDelta = pushDir * (pushSpeed * dt);
                        bool blockedByStatic = false;
                        Vec2 candidatePos = circle.pos + pushDelta;
                        for (const auto& box : config.staticBoxes) {
                            if (box.intersectCircle(candidatePos, circle.radius)) {
                                blockedByStatic = true;
                                break;
                            }
                        }
                        float maxMass = (circle.mass > 0.0f) ? circle.mass : 1.0f;
                        if (!blockedByStatic && tryPushDynamicChain(config.dynamicBoxes, config.dynamicVelocities,
                                                                    config.dynamicMasses, config.staticBoxes, i,
                                                                    pushDelta, pushDir, pushSpeed, maxMass)) {
                            circle.pos += pushDelta;
                        }
                    }

                    float relVel = (circle.velocity - config.dynamicVelocities[i]).dot(normal);
                    if (relVel < 0.0f) {
                        float restitution = circle.restitution;
                        if (i < config.dynamicRestitutions.size()) {
                            restitution = (circle.restitution + config.dynamicRestitutions[i]) * 0.5f;
                        }
                        float impulse = -(1.0f + restitution) * relVel / invMassSum;
                        Vec2 impulseVec = normal * impulse;
                        circle.velocity += impulseVec * invMassCircle;
                        config.dynamicVelocities[i] -= impulseVec * invMassBox;
                    }

                    float friction = (circle.friction + config.dynamicFrictions[i]) * 0.5f;
                    applyFriction(circle.velocity, normal, friction);
                    applyFriction(config.dynamicVelocities[i], -normal, friction);
                }
            }
        }
    }
}
