#pragma once
#include <DX3D/Math/Geometry.h>
namespace dx3d {
    enum class Suit { Spades = 0, Hearts = 1, Clubs = 2, Diamonds = 3 };
    enum class Rank { Ace = 0, Two = 1, Three = 2, Four = 3, Five = 4, Six = 5, Seven = 6, Eight = 7, Nine = 8, Ten = 9, Jack = 10, Queen = 11, King = 12 };

    class CardComponent {
    public:
        CardComponent(Suit suit, Rank rank) : m_suit(suit), m_rank(rank), m_faceUp(true) {}

        Suit getSuit() const { return m_suit; }
        Rank getRank() const { return m_rank; }

        // Face up/down functionality
        bool isFaceUp() const { return m_faceUp; }
        void setFaceUp(bool faceUp) { m_faceUp = faceUp; }

        int getFrameIndex() const {
            return static_cast<int>(m_rank) + (static_cast<int>(m_suit) * 13);
        }

        int getFrameX() const {
            return m_faceUp ? static_cast<int>(m_rank) : 0; // Card back at frame 0 when face down
        }

        int getFrameY() const {
            return m_faceUp ? static_cast<int>(m_suit) : 4; // Card back at row 4 when face down
        }

        std::wstring getCardName() const {
            if (!m_faceUp) return L"[Hidden]";

            std::wstring rankNames[] = { L"A", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"10", L"J", L"Q", L"K" };
            std::wstring suitNames[] = { L"♠", L"♥", L"♣", L"♦" };
            return rankNames[static_cast<int>(m_rank)] + suitNames[static_cast<int>(m_suit)];
        }

        // Utility methods for card games
        int getValue() const {
            // Ace = 1, Face cards = 10, others = face value
            if (m_rank == Rank::Ace) return 1;
            if (static_cast<int>(m_rank) >= 10) return 10;
            return static_cast<int>(m_rank) + 1;
        }

        bool isRed() const {
            return m_suit == Suit::Hearts || m_suit == Suit::Diamonds;
        }

        bool isBlack() const {
            return m_suit == Suit::Spades || m_suit == Suit::Clubs;
        }

        // For solitaire games - check if cards can be stacked
        bool canStackOn(const CardComponent& other) const {
            // Different colors and descending rank
            return isRed() != other.isRed() &&
                static_cast<int>(m_rank) == static_cast<int>(other.m_rank) - 1;
        }

        // For Spider Solitaire - same suit descending
        bool canStackOnSpider(const CardComponent& other) const {
            return m_suit == other.m_suit &&
                static_cast<int>(m_rank) == static_cast<int>(other.m_rank) - 1;
        }

    private:
        Suit m_suit;
        Rank m_rank;
        bool m_faceUp;
    };
}