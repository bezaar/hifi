//
//  Batch.cpp
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/14/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Batch.h"

#include <QDebug>

using namespace gpu;

Batch::Batch() :
    _commands(),
    _params(),
    _resources(){
}

Batch::~Batch() {
}

void Batch::clear() {
    _commands.clear();
    _params.clear();
    _resources.clear();
}

void Batch::draw( Primitive primitiveType, int nbVertices, int startVertex) {
    _commands.push(COMMAND_DRAW);
    _params.push(startVertex);
    _params.push(nbVertices);
    _params.push(primitiveType);
}

void Batch::drawIndexed( Primitive primitiveType, int nbIndices, int startIndex) {
    _commands.push(COMMAND_DRAW_INDEXED);
    _params.push(startIndex);
    _params.push(nbIndices);
    _params.push(primitiveType);
}

void Batch::drawInstanced( uint32 nbInstances, Primitive primitiveType, int nbVertices, int startVertex, int startInstance) {
    _commands.push(COMMAND_DRAW_INSTANCED);
    _params.push(startInstance);
    _params.push(startVertex);
    _params.push(nbVertices);
    _params.push(primitiveType);
    _params.push(nbInstances);
}

void Batch::drawIndexedInstanced( uint32 nbInstances, Primitive primitiveType, int nbIndices, int startIndex, int startInstance) {
    _commands.push(COMMAND_DRAW_INDEXED_INSTANCED);
    _params.push(startInstance);
    _params.push(startIndex);
    _params.push(nbIndices);
    _params.push(primitiveType);
    _params.push(nbInstances);
}



