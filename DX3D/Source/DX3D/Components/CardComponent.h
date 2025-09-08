#pragma once
#include <DX3D/Math/Geometry.h>

namespace dx3d {
    enum class Suit { Spades = 0, Hearts = 1, Clubs = 2, Diamonds = 3 };
    enum class Rank { Ace = 0, Two = 1, Three = 2, Four = 3, Five = 4, Six = 5, Seven = 6, Eight = 7, Nine = 8, Ten = 9, Jack = 10, Queen = 11, King = 12 };

    class CardComponent {
    public:
        CardComponent(Suit suit, Rank rank) : m_suit(suit), m_rank(rank) {}

        Suit getSuit() const { return m_suit; }
        Rank getRank() const { return m_rank; }

        int getFrameIndex() const {
            return static_cast<int>(m_rank) + (static_cast<int>(m_suit) * 13);
        }

        int getFrameX() const { return static_cast<int>(m_rank); }
        int getFrameY() const { return static_cast<int>(m_suit); }

        std::wstring getCardName() const {
            std::wstring rankNames[] = { L"A", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"10", L"J", L"Q", L"K" };
            std::wstring suitNames[] = { L"♠", L"♥", L"♣", L"♦" };
            return rankNames[static_cast<int>(m_rank)] + suitNames[static_cast<int>(m_suit)];
        }

    private:
        Suit m_suit;
        Rank m_rank;
    };
}