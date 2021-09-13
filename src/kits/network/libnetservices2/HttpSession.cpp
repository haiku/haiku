/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */


#include <HttpSession.h>

using namespace BPrivate::Network;


class BHttpSession::Impl {

};


BHttpSession::BHttpSession()
{
	fImpl = std::make_shared<BHttpSession::Impl>();
}


BHttpSession::~BHttpSession()
{

}


BHttpSession::BHttpSession(const BHttpSession&) noexcept = default;


BHttpSession&
BHttpSession::operator=(const BHttpSession&) noexcept = default;

