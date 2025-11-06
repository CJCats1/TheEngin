/*MIT License
... see repository root ...*/

#pragma once
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Components/FirmGuyComponent.h>
#include <DX3D/Components/SpringGuyComponent.h>
#include <DX3D/Components/SoftGuyComponent.h>

namespace dx3d {
    class FirmGuySystem {
    public:
        static void update(EntityManager& em, float dt) {
            // Use pixel-friendly gravity; clamp dt for stability
            const float gravity = -2000.0f; // pixels/s^2
            if (dt > 1.0f / 60.0f) dt = 1.0f / 60.0f;

            auto bodies = em.getEntitiesWithComponent<FirmGuyComponent>();
            auto springNodes = em.getEntitiesWithComponent<SpringGuyNodeComponent>();
            auto softGuyEntities = em.getEntitiesWithComponent<SoftGuyComponent>();

            // Sub-step integration to reduce tunneling through thin/rotating walls
            const int subSteps = 4;
            float subDt = dt / static_cast<float>(subSteps);
            for (int step = 0; step < subSteps; ++step) {
                // Integrate
                for (auto* e : bodies) {
                    auto* rb = e->getComponent<FirmGuyComponent>();
                    if (!rb || rb->isStatic()) continue;

                    Vec2 v = rb->getVelocity();
                    v.y += gravity * rb->getGravityScale() * subDt;
                    v *= rb->getFriction();
                    Vec2 p = rb->getPosition() + v * subDt;
                    rb->setVelocity(v);
                    rb->setPosition(p);
                }

                // Resolve collisions; perform a few solver iterations per substep
                const int solverIterations = 3;
                for (int it = 0; it < solverIterations; ++it) {
                    for (size_t i = 0; i < bodies.size(); ++i) {
                        for (size_t j = i + 1; j < bodies.size(); ++j) {
                    auto* ea = bodies[i];
                    auto* eb = bodies[j];
                    auto* a = ea->getComponent<FirmGuyComponent>();
                    auto* b = eb->getComponent<FirmGuyComponent>();
                    if (!a || !b) continue;

                    // Static-static: skip
                    if (a->isStatic() && b->isStatic()) continue;

                    if (a->getShape() == FirmGuyShape::Circle && b->getShape() == FirmGuyShape::Circle) {
                        Vec2 pa = a->getPosition();
                        Vec2 pb = b->getPosition();
                        Vec2 d = pb - pa;
                        float dist = d.length();
                        float r = a->getRadius() + b->getRadius();
                        if (dist > 0.0f && dist < r) {
                            Vec2 n = d / dist;
                            float penetration = r - dist;
                            // positional correction
                            float totalInvMass = (a->isStatic()?0.0f:1.0f/a->getMass()) + (b->isStatic()?0.0f:1.0f/b->getMass());
                            if (totalInvMass > 0.0f) {
                                Vec2 correction = n * (penetration / totalInvMass);
                                if (!a->isStatic()) a->setPosition(a->getPosition() - correction * (1.0f/a->getMass()));
                                if (!b->isStatic()) b->setPosition(b->getPosition() + correction * (1.0f/b->getMass()));
                            }
                            // velocity response (1D along normal)
                            Vec2 va = a->getVelocity();
                            Vec2 vb = b->getVelocity();
                            float vaN = va.dot(n);
                            float vbN = vb.dot(n);
                            float e = std::min(a->getRestitution(), b->getRestitution());
                            float invMa = a->isStatic()?0.0f:1.0f/a->getMass();
                            float invMb = b->isStatic()?0.0f:1.0f/b->getMass();
                            float jImpulse = -(1.0f + e) * (vbN - vaN) / (invMa + invMb);
                            if (!a->isStatic()) va += n * (jImpulse * invMa);
                            if (!b->isStatic()) vb -= n * (jImpulse * invMb);
                            a->setVelocity(va);
                            b->setVelocity(vb);
                        }
                    }
                    else if (a->getShape() == FirmGuyShape::Rectangle && b->getShape() == FirmGuyShape::Rectangle) {
                        Vec2 ha = a->getHalfExtents();
                        Vec2 hb = b->getHalfExtents();
                        Vec2 ca = a->getPosition();
                        Vec2 cb = b->getPosition();
                        
                        // For static bodies, use sprite position instead of physics position
                        if (a->isStatic()) {
                            if (auto* sprite = ea->getComponent<SpriteComponent>()) {
                                Vec3 spritePos = sprite->getPosition();
                                ca = Vec2(spritePos.x, spritePos.y);
                            }
                        }
                        if (b->isStatic()) {
                            if (auto* sprite = eb->getComponent<SpriteComponent>()) {
                                Vec3 spritePos = sprite->getPosition();
                                cb = Vec2(spritePos.x, spritePos.y);
                            }
                        }
                        Vec2 diff = cb - ca;
                        float overlapX = ha.x + hb.x - std::fabs(diff.x);
                        float overlapY = ha.y + hb.y - std::fabs(diff.y);
                        if (overlapX > 0 && overlapY > 0) {
                            // resolve along smallest axis
                            Vec2 n;
                            float penetration;
                            if (overlapX < overlapY) { n = Vec2(diff.x < 0 ? -1.0f : 1.0f, 0.0f); penetration = overlapX; }
                            else { n = Vec2(0.0f, diff.y < 0 ? -1.0f : 1.0f); penetration = overlapY; }
                            float totalInvMass = (a->isStatic()?0.0f:1.0f/a->getMass()) + (b->isStatic()?0.0f:1.0f/b->getMass());
                            if (totalInvMass > 0.0f) {
                                Vec2 correction = n * (penetration / totalInvMass);
                                if (!a->isStatic()) a->setPosition(a->getPosition() - correction * (1.0f/a->getMass()));
                                if (!b->isStatic()) b->setPosition(b->getPosition() + correction * (1.0f/b->getMass()));
                            }
                            // velocity response along n
                            Vec2 va = a->getVelocity();
                            Vec2 vb = b->getVelocity();
                            float vaN = va.dot(n);
                            float vbN = vb.dot(n);
                            float e = std::min(a->getRestitution(), b->getRestitution());
                            float invMa = a->isStatic()?0.0f:1.0f/a->getMass();
                            float invMb = b->isStatic()?0.0f:1.0f/b->getMass();
                            float jImpulse = -(1.0f + e) * (vbN - vaN) / (invMa + invMb);
                            if (!a->isStatic()) va += n * (jImpulse * invMa);
                            if (!b->isStatic()) vb -= n * (jImpulse * invMb);
                            a->setVelocity(va);
                            b->setVelocity(vb);
                        }
                    }
                    else {
                        // Handle circle-rectangle in either order (with rotation support + simple CCD)
                        FirmGuyComponent* circ = nullptr; FirmGuyComponent* rect = nullptr; Entity* rectEntity = nullptr;
                        if (a->getShape() == FirmGuyShape::Circle && b->getShape() == FirmGuyShape::Rectangle) { 
                            circ = a; rect = b; rectEntity = eb; 
                        }
                        else if (a->getShape() == FirmGuyShape::Rectangle && b->getShape() == FirmGuyShape::Circle) { 
                            circ = b; rect = a; rectEntity = ea; 
                        }
                        if (circ && rect) {
                            Vec2 rc = rect->getPosition();
                            Vec2 he = rect->getHalfExtents();

                            // Prefer sprite transform for static bodies to follow manual rotation/position
                            if (rect->isStatic()) {
                                if (auto* sprite = rectEntity->getComponent<SpriteComponent>()) {
                                    Vec3 sp = sprite->getPosition();
                                    rc = Vec2(sp.x, sp.y);
                                }
                            }
                            float rectAngle = rect->getAngle();
                            if (auto* sprite = rectEntity->getComponent<SpriteComponent>()) rectAngle = sprite->getRotationZ();

                            // CCD: sweep circle center from p0 to p1 in this substep against expanded OBB
                            Vec2 p0 = circ->getPosition();
                            Vec2 vstep = circ->getVelocity() * subDt;
                            Vec2 p1 = p0 + vstep;
                            float cosA = std::cos(-rectAngle), sinA = std::sin(-rectAngle);
                            auto toLocal = [&](const Vec2& w){ Vec2 d=w-rc; return Vec2(d.x*cosA - d.y*sinA, d.x*sinA + d.y*cosA); };
                            Vec2 lp0 = toLocal(p0);
                            Vec2 lp1 = toLocal(p1);
                            Vec2 ld = lp1 - lp0;
                            Vec2 ext = he + Vec2(circ->getRadius(), circ->getRadius());

                            float tEnter = 0.0f, tExit = 1.0f;
                            for (int ax=0; ax<2; ++ax) {
                                float p = (ax==0? lp0.x: lp0.y);
                                float d = (ax==0? ld.x: ld.y);
                                float minB = -((ax==0)? ext.x: ext.y);
                                float maxB =  ((ax==0)? ext.x: ext.y);
                                if (std::abs(d) < 1e-5f) {
                                    if (p < minB || p > maxB) { tEnter = 2.0f; break; }
                                } else {
                                    float invD = 1.0f/d;
                                    float t1 = (minB - p) * invD;
                                    float t2 = (maxB - p) * invD;
                                    if (t1 > t2) std::swap(t1, t2);
                                    tEnter = std::max(tEnter, t1);
                                    tExit  = std::min(tExit,  t2);
                                    if (tEnter > tExit) { tEnter = 2.0f; break; }
                                }
                            }

                            if (tEnter <= 1.0f) {
                                // Determine hit normal from which slab we entered
                                Vec2 hit = lp0 + ld * tEnter;
                                Vec2 nLocal(0,0);
                                float dx = ext.x - std::abs(hit.x);
                                float dy = ext.y - std::abs(hit.y);
                                if (dx < dy) nLocal = Vec2(hit.x > 0 ? 1.0f : -1.0f, 0.0f);
                                else         nLocal = Vec2(0.0f,     hit.y > 0 ? 1.0f : -1.0f);
                                float c = std::cos(rectAngle), s = std::sin(rectAngle);
                                Vec2 nWorld = Vec2(nLocal.x*c - nLocal.y*s, nLocal.x*s + nLocal.y*c);

                                Vec2 newPos = p0 + vstep * tEnter + nWorld * 0.5f; // bias out
                                circ->setPosition(newPos);
                                Vec2 vel = circ->getVelocity();
                                float vn = vel.dot(nWorld);
                                if (vn < 0.0f) vel -= nWorld * (1.0f + circ->getRestitution()) * vn;
                                circ->setVelocity(vel);
                            } else {
                                // Discrete fallback for near contacts
                                Vec2 cc = circ->getPosition();
                                Vec2 local = cc - rc;
                                float cosA2 = std::cos(-rectAngle);
                                float sinA2 = std::sin(-rectAngle);
                                Vec2 localRotated = Vec2(
                                    local.x * cosA2 - local.y * sinA2,
                                    local.x * sinA2 + local.y * cosA2
                                );
                                Vec2 closest = Vec2(
                                    clamp(localRotated.x, -he.x, he.x),
                                    clamp(localRotated.y, -he.y, he.y)
                                );
                                Vec2 closestWorld = Vec2(
                                    closest.x * cosA2 - closest.y * sinA2,
                                    closest.x * sinA2 + closest.y * cosA2
                                ) + rc;
                                Vec2 diff = cc - closestWorld;
                                float dist = diff.length();
                                float r = circ->getRadius();
                                if (dist < r && dist > 0.0f) {
                                    Vec2 n = diff / dist;
                                    float penetration = r - dist;
                                    float invMc = circ->isStatic()?0.0f:1.0f/circ->getMass();
                                    float invMr = rect->isStatic()?0.0f:1.0f/rect->getMass();
                                    float totalInvMass = invMc + invMr;
                                    if (totalInvMass > 0.0f) {
                                        Vec2 correction = n * (penetration / totalInvMass);
                                        if (!circ->isStatic()) circ->setPosition(circ->getPosition() + correction * invMc);
                                        if (!rect->isStatic()) rect->setPosition(rect->getPosition() - correction * invMr);
                                    }
                                    Vec2 vc = circ->getVelocity();
                                    float vcN = vc.dot(n);
                                    float e = circ->getRestitution();
                                    vc -= n * (1.0f + e) * std::min(vcN, 0.0f);
                                    circ->setVelocity(vc);
                                }
                            }
                        }
                        }
                    }
                    
                    // Collision detection between FirmGuy and SpringGuy nodes
                    for (auto* firmEntity : bodies) {
                        for (auto* springEntity : springNodes) {
                            auto* firmGuy = firmEntity->getComponent<FirmGuyComponent>();
                            auto* springNode = springEntity->getComponent<SpringGuyNodeComponent>();
                            if (!firmGuy || !springNode) continue;
                            
                            // Skip if both are static
                            if (firmGuy->isStatic() && springNode->isPositionFixed()) continue;
                            
                            Vec2 firmPos = firmGuy->getPosition();
                            Vec2 springPos = springNode->getPosition();
                            
                            if (firmGuy->getShape() == FirmGuyShape::Circle) {
                                // Circle vs Point collision (treating SpringGuy node as a small circle)
                                float nodeRadius = 14.0f; // Default node size
                                Vec2 diff = springPos - firmPos;
                                float dist = diff.length();
                                float r = firmGuy->getRadius() + nodeRadius;
                                
                                if (dist > 0.0f && dist < r) {
                                    Vec2 n = diff / dist;
                                    float penetration = r - dist;
                                    
                                    // Positional correction
                                    float totalInvMass = (firmGuy->isStatic() ? 0.0f : 1.0f / firmGuy->getMass()) + 
                                                        (springNode->isPositionFixed() ? 0.0f : 1.0f / 1.0f); // SpringGuy nodes have mass 1.0
                                    if (totalInvMass > 0.0f) {
                                        Vec2 correction = n * (penetration / totalInvMass);
                                        if (!firmGuy->isStatic()) firmGuy->setPosition(firmGuy->getPosition() - correction * (1.0f / firmGuy->getMass()));
                                        if (!springNode->isPositionFixed()) springNode->setPosition(springNode->getPosition() + correction * 1.0f);
                                    }
                                    
                                    // Velocity response
                                    Vec2 firmVel = firmGuy->getVelocity();
                                    Vec2 springVel = springNode->getVelocity();
                                    float firmVelN = firmVel.dot(n);
                                    float springVelN = springVel.dot(n);
                                    float e = firmGuy->getRestitution();
                                    float invMf = firmGuy->isStatic() ? 0.0f : 1.0f / firmGuy->getMass();
                                    float invMs = springNode->isPositionFixed() ? 0.0f : 1.0f;
                                    float jImpulse = -(1.0f + e) * (springVelN - firmVelN) / (invMf + invMs);
                                    
                                    if (!firmGuy->isStatic()) firmVel += n * (jImpulse * invMf);
                                    if (!springNode->isPositionFixed()) springVel -= n * (jImpulse * invMs);
                                    
                                    firmGuy->setVelocity(firmVel);
                                    springNode->setVelocity(springVel);
                                }
                            }
                            else if (firmGuy->getShape() == FirmGuyShape::Rectangle) {
                                // Rectangle vs Point collision
                                float nodeRadius = 14.0f;
                                Vec2 halfExtents = firmGuy->getHalfExtents();
                                Vec2 rectCenter = firmGuy->getPosition();
                                
                                // For static bodies, use sprite position instead of physics position
                                if (firmGuy->isStatic()) {
                                    if (auto* sprite = firmEntity->getComponent<SpriteComponent>()) {
                                        Vec3 spritePos = sprite->getPosition();
                                        rectCenter = Vec2(spritePos.x, spritePos.y);
                                    }
                                }
                                
                                Vec2 diff = springPos - rectCenter;
                                float rectAngle = firmGuy->getAngle();
                                if (auto* sprite = firmEntity->getComponent<SpriteComponent>()) rectAngle = sprite->getRotationZ();
                                
                                // Transform point to local rectangle space
                                float cosA = std::cos(-rectAngle);
                                float sinA = std::sin(-rectAngle);
                                Vec2 localPoint = Vec2(
                                    diff.x * cosA - diff.y * sinA,
                                    diff.x * sinA + diff.y * cosA
                                );
                                
                                // Check if point is inside expanded rectangle
                                Vec2 closest = Vec2(
                                    clamp(localPoint.x, -halfExtents.x, halfExtents.x),
                                    clamp(localPoint.y, -halfExtents.y, halfExtents.y)
                                );
                                Vec2 closestWorld = Vec2(
                                    closest.x * cosA - closest.y * sinA,
                                    closest.x * sinA + closest.y * cosA
                                ) + rectCenter;
                                
                                Vec2 separation = springPos - closestWorld;
                                float dist = separation.length();
                                
                                if (dist < nodeRadius && dist > 0.0f) {
                                    Vec2 n = separation / dist;
                                    float penetration = nodeRadius - dist;
                                    
                                    // Positional correction
                                    float totalInvMass = (firmGuy->isStatic() ? 0.0f : 1.0f / firmGuy->getMass()) + 
                                                        (springNode->isPositionFixed() ? 0.0f : 1.0f);
                                    if (totalInvMass > 0.0f) {
                                        Vec2 correction = n * (penetration / totalInvMass);
                                        if (!firmGuy->isStatic()) firmGuy->setPosition(firmGuy->getPosition() - correction * (1.0f / firmGuy->getMass()));
                                        if (!springNode->isPositionFixed()) springNode->setPosition(springNode->getPosition() + correction * 1.0f);
                                    }
                                    
                                    // Velocity response
                                    Vec2 firmVel = firmGuy->getVelocity();
                                    Vec2 springVel = springNode->getVelocity();
                                    float firmVelN = firmVel.dot(n);
                                    float springVelN = springVel.dot(n);
                                    float e = firmGuy->getRestitution();
                                    float invMf = firmGuy->isStatic() ? 0.0f : 1.0f / firmGuy->getMass();
                                    float invMs = springNode->isPositionFixed() ? 0.0f : 1.0f;
                                    float jImpulse = -(1.0f + e) * (springVelN - firmVelN) / (invMf + invMs);
                                    
                                    if (!firmGuy->isStatic()) firmVel += n * (jImpulse * invMf);
                                    if (!springNode->isPositionFixed()) springVel -= n * (jImpulse * invMs);
                                    
                                    firmGuy->setVelocity(firmVel);
                                    springNode->setVelocity(springVel);
                                }
                            }
                        }
                    }
                    
                    // Collision detection between FirmGuy and SoftGuy
                    for (auto* firmEntity : bodies) {
                        for (auto* softEntity : softGuyEntities) {
                            auto* firmGuy = firmEntity->getComponent<FirmGuyComponent>();
                            auto* softGuy = softEntity->getComponent<SoftGuyComponent>();
                            if (!firmGuy || !softGuy) continue;
                            
                            // Get SoftGuy nodes for collision detection
                            const auto& softNodes = softGuy->getNodes();
                            
                            for (auto* softNodeEntity : softNodes) {
                                auto* softNode = softNodeEntity->getComponent<SpringGuyNodeComponent>();
                                if (!softNode) continue;
                                
                                // Skip if both are static
                                if (firmGuy->isStatic() && softNode->isPositionFixed()) continue;
                                
                                Vec2 firmPos = firmGuy->getPosition();
                                Vec2 softPos = softNode->getPosition();
                                
                                if (firmGuy->getShape() == FirmGuyShape::Circle) {
                                    // Circle vs Point collision
                                    float nodeRadius = 14.0f;
                                    Vec2 diff = softPos - firmPos;
                                    float dist = diff.length();
                                    float r = firmGuy->getRadius() + nodeRadius;
                                    
                                    if (dist > 0.0f && dist < r) {
                                        Vec2 n = diff / dist;
                                        float penetration = r - dist;
                                        
                                        // Positional correction
                                        float totalInvMass = (firmGuy->isStatic() ? 0.0f : 1.0f / firmGuy->getMass()) + 
                                                            (softNode->isPositionFixed() ? 0.0f : 1.0f);
                                        if (totalInvMass > 0.0f) {
                                            Vec2 correction = n * (penetration / totalInvMass);
                                            if (!firmGuy->isStatic()) firmGuy->setPosition(firmGuy->getPosition() - correction * (1.0f / firmGuy->getMass()));
                                            if (!softNode->isPositionFixed()) softNode->setPosition(softNode->getPosition() + correction * 1.0f);
                                        }
                                        
                                        // Velocity response
                                        Vec2 firmVel = firmGuy->getVelocity();
                                        Vec2 softVel = softNode->getVelocity();
                                        float firmVelN = firmVel.dot(n);
                                        float softVelN = softVel.dot(n);
                                        float e = firmGuy->getRestitution();
                                        float invMf = firmGuy->isStatic() ? 0.0f : 1.0f / firmGuy->getMass();
                                        float invMs = softNode->isPositionFixed() ? 0.0f : 1.0f;
                                        float jImpulse = -(1.0f + e) * (softVelN - firmVelN) / (invMf + invMs);
                                        
                                        if (!firmGuy->isStatic()) firmVel += n * (jImpulse * invMf);
                                        if (!softNode->isPositionFixed()) softVel -= n * (jImpulse * invMs);
                                        
                                        firmGuy->setVelocity(firmVel);
                                        softNode->setVelocity(softVel);
                                    }
                                }
                                else if (firmGuy->getShape() == FirmGuyShape::Rectangle) {
                                    // Rectangle vs Point collision
                                    float nodeRadius = 14.0f;
                                    Vec2 halfExtents = firmGuy->getHalfExtents();
                                    Vec2 rectCenter = firmGuy->getPosition();
                                    
                                    // For static bodies, use sprite position instead of physics position
                                    if (firmGuy->isStatic()) {
                                        if (auto* sprite = firmEntity->getComponent<SpriteComponent>()) {
                                            Vec3 spritePos = sprite->getPosition();
                                            rectCenter = Vec2(spritePos.x, spritePos.y);
                                        }
                                    }
                                    
                                    Vec2 diff = softPos - rectCenter;
                                    float rectAngle = firmGuy->getAngle();
                                    if (auto* sprite = firmEntity->getComponent<SpriteComponent>()) rectAngle = sprite->getRotationZ();
                                    
                                    // Transform point to local rectangle space
                                    float cosA = std::cos(-rectAngle);
                                    float sinA = std::sin(-rectAngle);
                                    Vec2 localPoint = Vec2(
                                        diff.x * cosA - diff.y * sinA,
                                        diff.x * sinA + diff.y * cosA
                                    );
                                    
                                    // Check if point is inside expanded rectangle
                                    Vec2 closest = Vec2(
                                        clamp(localPoint.x, -halfExtents.x, halfExtents.x),
                                        clamp(localPoint.y, -halfExtents.y, halfExtents.y)
                                    );
                                    Vec2 closestWorld = Vec2(
                                        closest.x * cosA - closest.y * sinA,
                                        closest.x * sinA + closest.y * cosA
                                    ) + rectCenter;
                                    
                                    Vec2 separation = softPos - closestWorld;
                                    float dist = separation.length();
                                    
                                    if (dist < nodeRadius && dist > 0.0f) {
                                        Vec2 n = separation / dist;
                                        float penetration = nodeRadius - dist;
                                        
                                        // Positional correction
                                        float totalInvMass = (firmGuy->isStatic() ? 0.0f : 1.0f / firmGuy->getMass()) + 
                                                            (softNode->isPositionFixed() ? 0.0f : 1.0f);
                                        if (totalInvMass > 0.0f) {
                                            Vec2 correction = n * (penetration / totalInvMass);
                                            if (!firmGuy->isStatic()) firmGuy->setPosition(firmGuy->getPosition() - correction * (1.0f / firmGuy->getMass()));
                                            if (!softNode->isPositionFixed()) softNode->setPosition(softNode->getPosition() + correction * 1.0f);
                                        }
                                        
                                        // Velocity response
                                        Vec2 firmVel = firmGuy->getVelocity();
                                        Vec2 softVel = softNode->getVelocity();
                                        float firmVelN = firmVel.dot(n);
                                        float softVelN = softVel.dot(n);
                                        float e = firmGuy->getRestitution();
                                        float invMf = firmGuy->isStatic() ? 0.0f : 1.0f / firmGuy->getMass();
                                        float invMs = softNode->isPositionFixed() ? 0.0f : 1.0f;
                                        float jImpulse = -(1.0f + e) * (softVelN - firmVelN) / (invMf + invMs);
                                        
                                        if (!firmGuy->isStatic()) firmVel += n * (jImpulse * invMf);
                                        if (!softNode->isPositionFixed()) softVel -= n * (jImpulse * invMs);
                                        
                                        firmGuy->setVelocity(firmVel);
                                        softNode->setVelocity(softVel);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            }

            // Sync to sprites if present (position only; keep authoring size)
            // Skip static bodies that are manually positioned (like rotating box walls)
            for (auto* e : bodies) {
                auto* rb = e->getComponent<FirmGuyComponent>();
                if (!rb) continue;
                if (auto* sprite = e->getComponent<SpriteComponent>()) {
                    // Only sync position for non-static bodies or if sprite position differs significantly from physics position
                    Vec2 physicsPos = rb->getPosition();
                    Vec3 spritePos = sprite->getPosition();
                    Vec2 spritePos2D = Vec2(spritePos.x, spritePos.y);
                    float distance = (physicsPos - spritePos2D).length();
                    
                    if (!rb->isStatic() || distance > 1.0f) {
                        sprite->setPosition(physicsPos.x, physicsPos.y, 0.0f);
                    }
                }
            }
        }
    };
}


