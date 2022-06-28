////////////////////////////////////////////////////////////////////////////
//	Module 		: trade_factors_inline.h
//	Created 	: 13.01.2006
//  Modified 	: 13.01.2006
//	Author		: Dmitriy Iassenev
//	Description : trade factors class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC CTradeFactors::CTradeFactors ( const float &friend_factor, const float &enemy_factor, const float &min_condition, const bool &disabled )
{
	VERIFY			(_valid(friend_factor));
	m_friend_factor	= friend_factor;

	VERIFY			(_valid(enemy_factor));
	m_enemy_factor	= enemy_factor;

	VERIFY( _valid( min_condition ) );
	m_min_condition	= min_condition;

	m_disabled = disabled;
}

IC	const float &CTradeFactors::friend_factor	() const
{
	VERIFY			(_valid(m_friend_factor));
	return			(m_friend_factor);
}

IC	const float &CTradeFactors::enemy_factor	() const
{
	VERIFY			(_valid(m_enemy_factor));
	return			(m_enemy_factor);
}


IC const float &CTradeFactors::min_condition() const {
  VERIFY( _valid( m_min_condition ) );
  return m_min_condition;
}


IC const bool &CTradeFactors::disabled() const {
  return m_disabled;
}
