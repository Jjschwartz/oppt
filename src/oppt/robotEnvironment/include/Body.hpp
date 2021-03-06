/**
 * Copyright 2017
 *
 * This file is part of On-line POMDP Planning Toolkit (OPPT).
 * OPPT is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License published by the Free Software Foundation,
 * either version 2 of the License, or (at your option) any later version.
 *
 * OPPT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with OPPT.
 * If not, see http://www.gnu.org/licenses/.
 */
/** @file ManipulatorState.hpp
 *
 * Defines the RockSampleState class, which represents a state of the RockSample problem.
 */
#ifndef BODY_HPP_
#define BODY_HPP_
#include "fcl/BV/BV.h"
#include "fcl/collision_object.h"
#include "fcl/collision_data.h"
#include "fcl/distance.h"
#include "fcl/shape/geometric_shapes.h"
#include "fcl/shape/geometric_shapes_utility.h"
#include "oppt/opptCore/core.hpp"

using std::cout;
using std::endl;

namespace oppt
{

/**
 * Concrete implementation of oppt::Body
 */
class BodyImpl: public oppt::Body
{
public:
    BodyImpl(const std::string& name, const geometric::Pose &worldPose);

    virtual ~BodyImpl() = default;

    virtual void updateCollisionObject() override;

    virtual void initVisualGeometry(const GeometrySharedPtr &visualGeometry) override;

    /**virtual bool collides(const std::vector<oppt::BodySharedPtr>& otherBodies) const override;

    virtual bool collides(const std::vector<oppt::CollisionObjectSharedPtr>& collisionObjects,
                          unsigned int& collidingBodyIndex) const override;

    virtual bool collides(const VectorFloat& point) const override;*/

    //virtual FloatType distance(const std::vector<oppt::CollisionObjectSharedPtr>& collisionObjects) const override;

    //virtual bool collidesContinuous(const oppt::CollisionObjectSharedPtr& collisionObjectStart,
    //                                const oppt::CollisionObjectSharedPtr& collisionObjectGoal) const override;

    virtual std::string toSDFString() const override;

protected:
    virtual void createCollisionObject() override;
};

}

#endif /* BODY_HPP_ */
