// -----------------------------------------------------------------------------------------------------
// Copyright (c) 2006-2021, Knut Reinert & Freie Universität Berlin
// Copyright (c) 2016-2021, Knut Reinert & MPI für molekulare Genetik
// This file may be used, modified and/or redistributed under the terms of the 3-clause BSD-License
// shipped with this file and also available at: https://github.com/seqan/seqan3/blob/master/LICENSE.md
// -----------------------------------------------------------------------------------------------------

/*!\file
 * \author Mitra Darvish <mitra.darvish AT fu-berlin.de>
 * \brief Provides seqan3::views::minimiser.
 */

#pragma once

#include <algorithm>
#include <deque>

#include <seqan3/core/detail/empty_type.hpp>
#include <seqan3/core/range/detail/adaptor_from_functor.hpp>
#include <seqan3/core/range/type_traits.hpp>
#include <seqan3/utility/range/concept.hpp>
#include <seqan3/utility/type_traits/lazy_conditional.hpp>

namespace seqan3::detail
{
// ---------------------------------------------------------------------------------------------------------------------
// minimiser_view class
// ---------------------------------------------------------------------------------------------------------------------

/*!\brief The type returned by seqan3::views::minimiser.
 * \tparam urng1_t The type of the underlying range, must model std::ranges::forward_range, the reference type must
 *                 model std::totally_ordered. The typical use case is that the reference type is the result of
 *                 seqan3::kmer_hash.
 * \tparam urng2_t The type of the second underlying range, must model std::ranges::forward_range, the reference type
 *                 must model std::totally_ordered. If only one range is provided this defaults to
 *                 std::ranges::empty_view.
 * \implements std::ranges::view
 * \ingroup search_views
 *
 * \details
 *
 * See seqan3::views::minimiser for a detailed explanation on minimizers.
 *
 * \note Most members of this class are generated by std::ranges::view_interface which is not yet documented here.
 *
 * \sa seqan3::views::minimiser
 */
template <std::ranges::view urng1_t, std::ranges::view urng2_t = std::ranges::empty_view<seqan3::detail::empty_type>>
class minimiser_view : public std::ranges::view_interface<minimiser_view<urng1_t, urng2_t>>
{
private:
    static_assert(std::ranges::forward_range<urng1_t>, "The minimiser_view only works on forward_ranges.");
    static_assert(std::ranges::forward_range<urng2_t>, "The minimiser_view only works on forward_ranges.");
    static_assert(std::totally_ordered<std::ranges::range_reference_t<urng1_t>>,
                  "The reference type of the underlying range must model std::totally_ordered.");

    //!\brief The default argument of the second range.
    using default_urng2_t = std::ranges::empty_view<seqan3::detail::empty_type>;

    //!\brief Boolean variable, which is true, when second range is not of empty type.
    static constexpr bool second_range_is_given = !std::same_as<urng2_t, default_urng2_t>;

    static_assert(!second_range_is_given
                      || std::totally_ordered_with<std::ranges::range_reference_t<urng1_t>,
                                                   std::ranges::range_reference_t<urng2_t>>,
                  "The reference types of the underlying ranges must model std::totally_ordered_with.");

    //!\brief Whether the given ranges are const_iterable
    static constexpr bool const_iterable =
        seqan3::const_iterable_range<urng1_t> && seqan3::const_iterable_range<urng2_t>;

    //!\brief The first underlying range.
    urng1_t urange1{};
    //!\brief The second underlying range.
    urng2_t urange2{};

    //!\brief The number of values in one window.
    size_t window_size{};

    template <bool const_range>
    class basic_iterator;

    //!\brief The sentinel type of the minimiser_view.
    using sentinel = std::default_sentinel_t;

public:
    /*!\name Constructors, destructor and assignment
     * \{
     */
    minimiser_view()
        requires std::default_initializable<urng1_t> && std::default_initializable<urng2_t>
    = default;                                                        //!< Defaulted.
    minimiser_view(minimiser_view const & rhs) = default;             //!< Defaulted.
    minimiser_view(minimiser_view && rhs) = default;                  //!< Defaulted.
    minimiser_view & operator=(minimiser_view const & rhs) = default; //!< Defaulted.
    minimiser_view & operator=(minimiser_view && rhs) = default;      //!< Defaulted.
    ~minimiser_view() = default;                                      //!< Defaulted.

    /*!\brief Construct from a view and a given number of values in one window.
    * \param[in] urange1     The input range to process. Must model std::ranges::viewable_range and
    *                        std::ranges::forward_range.
    * \param[in] window_size The number of values in one window.
    */
    minimiser_view(urng1_t urange1, size_t const window_size) :
        minimiser_view{std::move(urange1), default_urng2_t{}, window_size}
    {}

    /*!\brief Construct from a non-view that can be view-wrapped and a given number of values in one window.
    * \tparam other_urng1_t  The type of another urange. Must model std::ranges::viewable_range and be constructible
                             from urng1_t.
    * \param[in] urange1     The input range to process. Must model std::ranges::viewable_range and
    *                        std::ranges::forward_range.
    * \param[in] window_size The number of values in one window.
    */
    template <typename other_urng1_t>
        requires (std::ranges::viewable_range<other_urng1_t>
                  && std::constructible_from<urng1_t, ranges::ref_view<std::remove_reference_t<other_urng1_t>>>)
    minimiser_view(other_urng1_t && urange1, size_t const window_size) :
        urange1{std::views::all(std::forward<other_urng1_t>(urange1))},
        urange2{default_urng2_t{}},
        window_size{window_size}
    {}

    /*!\brief Construct from two views and a given number of values in one window.
    * \param[in] urange1     The first input range to process. Must model std::ranges::viewable_range and
    *                        std::ranges::forward_range.
    * \param[in] urange2     The second input range to process. Must model std::ranges::viewable_range and
    *                        std::ranges::forward_range.
    * \param[in] window_size The number of values in one window.
    */
    minimiser_view(urng1_t urange1, urng2_t urange2, size_t const window_size) :
        urange1{std::move(urange1)},
        urange2{std::move(urange2)},
        window_size{window_size}
    {
        if constexpr (second_range_is_given)
        {
            if (std::ranges::distance(urange1) != std::ranges::distance(urange2))
                throw std::invalid_argument{"The two ranges do not have the same size."};
        }
    }

    /*!\brief Construct from two non-views that can be view-wrapped and a given number of values in one window.
    * \tparam other_urng1_t  The type of another urange. Must model std::ranges::viewable_range and be constructible
                             from urng1_t.
    * \tparam other_urng2_t  The type of another urange. Must model std::ranges::viewable_range and be constructible
                             from urng2_t.
    * \param[in] urange1     The input range to process. Must model std::ranges::viewable_range and
    *                        std::ranges::forward_range.
    * \param[in] urange2     The second input range to process. Must model std::ranges::viewable_range and
    *                        std::ranges::forward_range.
    * \param[in] window_size The number of values in one window.
    */
    template <typename other_urng1_t, typename other_urng2_t>
        requires (std::ranges::viewable_range<other_urng1_t>
                  && std::constructible_from<urng1_t, std::views::all_t<other_urng1_t>>
                  && std::ranges::viewable_range<other_urng2_t>
                  && std::constructible_from<urng2_t, std::views::all_t<other_urng2_t>>)
    minimiser_view(other_urng1_t && urange1, other_urng2_t && urange2, size_t const window_size) :
        urange1{std::views::all(std::forward<other_urng1_t>(urange1))},
        urange2{std::views::all(std::forward<other_urng2_t>(urange2))},
        window_size{window_size}
    {
        if constexpr (second_range_is_given)
        {
            if (std::ranges::distance(urange1) != std::ranges::distance(urange2))
                throw std::invalid_argument{"The two ranges do not have the same size."};
        }
    }
    //!\}

    /*!\name Iterators
     * \{
     */
    /*!\brief Returns an iterator to the first element of the range.
     * \returns Iterator to the first element.
     *
     * \details
     *
     * ### Complexity
     *
     * Constant.
     *
     * ### Exceptions
     *
     * Strong exception guarantee.
     */
    basic_iterator<false> begin()
    {
        return {std::ranges::begin(urange1), std::ranges::end(urange1), std::ranges::begin(urange2), window_size};
    }

    //!\copydoc begin()
    basic_iterator<true> begin() const
        requires const_iterable
    {
        return {std::ranges::cbegin(urange1), std::ranges::cend(urange1), std::ranges::cbegin(urange2), window_size};
    }

    /*!\brief Returns an iterator to the element following the last element of the range.
     * \returns Iterator to the end.
     *
     * \details
     *
     * This element acts as a placeholder; attempting to dereference it results in undefined behaviour.
     *
     * ### Complexity
     *
     * Constant.
     *
     * ### Exceptions
     *
     * No-throw guarantee.
     */
    sentinel end() const
    {
        return {};
    }
    //!\}
};

//!\brief Iterator for calculating minimisers.
template <std::ranges::view urng1_t, std::ranges::view urng2_t>
template <bool const_range>
class minimiser_view<urng1_t, urng2_t>::basic_iterator
{
private:
    //!\brief The sentinel type of the first underlying range.
    using urng1_sentinel_t = maybe_const_sentinel_t<const_range, urng1_t>;
    //!\brief The iterator type of the first underlying range.
    using urng1_iterator_t = maybe_const_iterator_t<const_range, urng1_t>;
    //!\brief The iterator type of the second underlying range.
    using urng2_iterator_t = maybe_const_iterator_t<const_range, urng2_t>;

    template <bool>
    friend class basic_iterator;

public:
    /*!\name Associated types
     * \{
     */
    //!\brief Type for distances between iterators.
    using difference_type = std::ranges::range_difference_t<urng1_t>;
    //!\brief Value type of this iterator.
    using value_type = std::ranges::range_value_t<urng1_t>;
    //!\brief The pointer type.
    using pointer = void;
    //!\brief Reference to `value_type`.
    using reference = value_type;
    //!\brief Tag this class as a forward iterator.
    using iterator_category = std::forward_iterator_tag;
    //!\brief Tag this class as a forward iterator.
    using iterator_concept = iterator_category;
    //!\}

    /*!\name Constructors, destructor and assignment
     * \{
     */
    basic_iterator() = default;                                   //!< Defaulted.
    basic_iterator(basic_iterator const &) = default;             //!< Defaulted.
    basic_iterator(basic_iterator &&) = default;                  //!< Defaulted.
    basic_iterator & operator=(basic_iterator const &) = default; //!< Defaulted.
    basic_iterator & operator=(basic_iterator &&) = default;      //!< Defaulted.
    ~basic_iterator() = default;                                  //!< Defaulted.

    //!\brief Allow iterator on a const range to be constructible from an iterator over a non-const range.
    basic_iterator(basic_iterator<!const_range> const & it)
        requires const_range
    :
        minimiser_value{std::move(it.minimiser_value)},
        urng1_iterator{std::move(it.urng1_iterator)},
        urng1_sentinel{std::move(it.urng1_sentinel)},
        urng2_iterator{std::move(it.urng2_iterator)},
        window_values{std::move(it.window_values)}
    {}

    /*!\brief Construct from begin and end iterators of a given range over std::totally_ordered values, and the number
              of values per window.
    * \param[in] urng1_iterator Iterator pointing to the first position of the first std::totally_ordered range.
    * \param[in] urng1_sentinel Iterator pointing to the last position of the first std::totally_ordered range.
    * \param[in] urng2_iterator Iterator pointing to the first position of the second std::totally_ordered range.
    * \param[in] window_size The number of values in one window.
    *
    * \details
    *
    * Looks at the number of values per window in two ranges, returns the smallest between both as minimiser and
    * shifts then by one to repeat this action. If a minimiser in consecutive windows is the same, it is returned only
    * once.
    */
    basic_iterator(urng1_iterator_t urng1_iterator,
                   urng1_sentinel_t urng1_sentinel,
                   urng2_iterator_t urng2_iterator,
                   size_t window_size) :
        urng1_iterator{std::move(urng1_iterator)},
        urng1_sentinel{std::move(urng1_sentinel)},
        urng2_iterator{std::move(urng2_iterator)}
    {
        size_t size = std::ranges::distance(urng1_iterator, urng1_sentinel);
        window_size = std::min<size_t>(window_size, size);

        window_first(window_size);
    }
    //!\}

    //!\anchor basic_iterator_comparison
    //!\name Comparison operators
    //!\{

    //!\brief Compare to another basic_iterator.
    friend bool operator==(basic_iterator const & lhs, basic_iterator const & rhs)
    {
        return (lhs.urng1_iterator == rhs.urng1_iterator) && (rhs.urng2_iterator == rhs.urng2_iterator)
            && (lhs.window_values.size() == rhs.window_values.size());
    }

    //!\brief Compare to another basic_iterator.
    friend bool operator!=(basic_iterator const & lhs, basic_iterator const & rhs)
    {
        return !(lhs == rhs);
    }

    //!\brief Compare to the sentinel of the minimiser_view.
    friend bool operator==(basic_iterator const & lhs, sentinel const &)
    {
        return lhs.urng1_iterator == lhs.urng1_sentinel;
    }

    //!\brief Compare to the sentinel of the minimiser_view.
    friend bool operator==(sentinel const & lhs, basic_iterator const & rhs)
    {
        return rhs == lhs;
    }

    //!\brief Compare to the sentinel of the minimiser_view.
    friend bool operator!=(sentinel const & lhs, basic_iterator const & rhs)
    {
        return !(lhs == rhs);
    }

    //!\brief Compare to the sentinel of the minimiser_view.
    friend bool operator!=(basic_iterator const & lhs, sentinel const & rhs)
    {
        return !(lhs == rhs);
    }
    //!\}

    //!\brief Pre-increment.
    basic_iterator & operator++() noexcept
    {
        next_unique_minimiser();
        return *this;
    }

    //!\brief Post-increment.
    basic_iterator operator++(int) noexcept
    {
        basic_iterator tmp{*this};
        next_unique_minimiser();
        return tmp;
    }

    //!\brief Return the minimiser.
    value_type operator*() const noexcept
    {
        return minimiser_value;
    }

    //!\brief Return the underlying iterator. It will point to the last element in the current window.
    constexpr urng1_iterator_t const & base() const & noexcept
    {
        return urng1_iterator;
    }

    //!\brief Return the underlying iterator. It will point to the last element in the current window.
    constexpr urng1_iterator_t base() &&
    {
        return std::move(urng1_iterator);
    }

private:
    //!\brief The minimiser value.
    value_type minimiser_value{};

    //!\brief The offset relative to the beginning of the window where the minimizer value is found.
    size_t minimiser_position_offset{};

    //!\brief Iterator to the rightmost value of one window.
    urng1_iterator_t urng1_iterator{};
    //!brief Iterator to last element in range.
    urng1_sentinel_t urng1_sentinel{};
    //!\brief Iterator to the rightmost value of one window of the second range.
    urng2_iterator_t urng2_iterator{};

    //!\brief Stored values per window. It is necessary to store them, because a shift can remove the current minimiser.
    std::deque<value_type> window_values{};

    //!\brief Increments iterator by 1.
    void next_unique_minimiser()
    {
        while (!next_minimiser())
        {}
    }

    //!\brief Returns new window value.
    auto window_value() const
    {
        if constexpr (!second_range_is_given)
            return *urng1_iterator;
        else
            return std::min(*urng1_iterator, *urng2_iterator);
    }

    //!\brief Advances the window to the next position.
    void advance_window()
    {
        ++urng1_iterator;
        if constexpr (second_range_is_given)
            ++urng2_iterator;
    }

    //!\brief Calculates minimisers for the first window.
    void window_first(size_t const window_size)
    {
        if (window_size == 0u)
            return;

        for (size_t i = 0u; i < window_size - 1u; ++i)
        {
            window_values.push_back(window_value());
            advance_window();
        }
        window_values.push_back(window_value());
        auto minimiser_it = std::ranges::min_element(window_values, std::less_equal<value_type>{});
        minimiser_value = *minimiser_it;
        minimiser_position_offset = std::distance(std::begin(window_values), minimiser_it);
    }

    /*!\brief Calculates the next minimiser value.
     * \returns True, if new minimiser is found or end is reached. Otherwise returns false.
     * \details
     * For the following windows, we remove the first window value (is now not in window_values) and add the new
     * value that results from the window shifting.
     */
    bool next_minimiser()
    {
        advance_window();
        if (urng1_iterator == urng1_sentinel)
            return true;

        value_type const new_value = window_value();

        window_values.pop_front();
        window_values.push_back(new_value);

        if (minimiser_position_offset == 0)
        {
            auto minimiser_it = std::ranges::min_element(window_values, std::less_equal<value_type>{});
            minimiser_value = *minimiser_it;
            minimiser_position_offset = std::distance(std::begin(window_values), minimiser_it);
            return true;
        }

        if (new_value < minimiser_value)
        {
            minimiser_value = new_value;
            minimiser_position_offset = window_values.size() - 1;
            return true;
        }

        --minimiser_position_offset;
        return false;
    }
};

//!\brief A deduction guide for the view class template.
template <std::ranges::viewable_range rng1_t>
minimiser_view(rng1_t &&, size_t const window_size) -> minimiser_view<std::views::all_t<rng1_t>>;

//!\brief A deduction guide for the view class template.
template <std::ranges::viewable_range rng1_t, std::ranges::viewable_range rng2_t>
minimiser_view(rng1_t &&, rng2_t &&, size_t const window_size)
    -> minimiser_view<std::views::all_t<rng1_t>, std::views::all_t<rng2_t>>;

// ---------------------------------------------------------------------------------------------------------------------
// minimiser_fn (adaptor definition)
// ---------------------------------------------------------------------------------------------------------------------

//![adaptor_def]
//!\brief seqan3::views::minimiser's range adaptor object type (non-closure).
//!\ingroup search_views
struct minimiser_fn
{
    //!\brief Store the number of values in one window and return a range adaptor closure object.
    constexpr auto operator()(size_t const window_size) const
    {
        return adaptor_from_functor{*this, window_size};
    }

    /*!\brief Call the view's constructor with two arguments: the underlying view and an integer indicating how many
     *        values one window contains.
     * \tparam urng1_t        The type of the input range to process. Must model std::ranges::viewable_range.
     * \param[in] urange1     The input range to process. Must model std::ranges::viewable_range and
     *                        std::ranges::forward_range.
     * \param[in] window_size The number of values in one window.
     * \returns  A range of converted values.
     */
    template <std::ranges::range urng1_t>
    constexpr auto operator()(urng1_t && urange1, size_t const window_size) const
    {
        static_assert(std::ranges::viewable_range<urng1_t>,
                      "The range parameter to views::minimiser cannot be a temporary of a non-view range.");
        static_assert(std::ranges::forward_range<urng1_t>,
                      "The range parameter to views::minimiser must model std::ranges::forward_range.");

        if (window_size == 1) // Would just return urange1 without any changes
            throw std::invalid_argument{"The chosen window_size is not valid. "
                                        "Please choose a value greater than 1 or use two ranges."};

        return minimiser_view{urange1, window_size};
    }
};
//![adaptor_def]

} // namespace seqan3::detail

namespace seqan3::views
{
/*!\brief Computes minimisers for a range of comparable values. A minimiser is the smallest value in a window.
 * \tparam urng_t The type of the first range being processed. See below for requirements. [template
 *                 parameter is omitted in pipe notation]
 * \param[in] urange1 The range being processed. [parameter is omitted in pipe notation]
 * \param[in] window_size The number of values in one window.
 * \returns A range of std::totally_ordered where each value is the minimal value for one window. See below for the
 *          properties of the returned range.
 * \ingroup search_views
 *
 * \details
 *
 * A minimiser is the smallest value in a window. For example for the following list of hash values
 * `[28, 100, 9, 23, 4, 1, 72, 37, 8]` and 4 as `window_size`, the minimiser values are `[9, 4, 1]`.
 *
 * The minimiser can be calculated for one given range or for two given ranges, where the minimizer is the smallest
 * value in both windows. For example for the following list of hash values `[28, 100, 9, 23, 4, 1, 72, 37, 8]` and
 * `[30, 2, 11, 101, 199, 73, 34, 900]` and 4 as `window_size`, the minimiser values are `[2, 4, 1]`.
 *
 * Note that in the interface with the second underlying range the const-iterable property will only be preserved if
 * both underlying ranges are const-iterable.
 *
 * ### Robust Winnowing
 *
 * In case there are multiple minimal values within one window, the minimum and therefore the minimiser is ambiguous.
 * We choose the rightmost value as the minimiser of the window, and when shifting the window, the minimiser is only
 * changed if there appears a value that is strictly smaller than the current minimum. This approach is termed
 * *robust winnowing* by [Chirag et al.](https://www.biorxiv.org/content/10.1101/2020.02.11.943241v1.full.pdf)
 * and is proven to work especially well on repeat regions.
 *
 * ### Example
 *
 * \include test/snippet/search/views/minimiser.cpp
 *
 * ### View properties
 *
 * | Concepts and traits              | `urng_t` (underlying range type)   | `rrng_t` (returned range type)   |
 * |----------------------------------|:----------------------------------:|:--------------------------------:|
 * | std::ranges::input_range         | *required*                         | *preserved*                      |
 * | std::ranges::forward_range       | *required*                         | *preserved*                      |
 * | std::ranges::bidirectional_range |                                    | *lost*                           |
 * | std::ranges::random_access_range |                                    | *lost*                           |
 * | std::ranges::contiguous_range    |                                    | *lost*                           |
 * |                                  |                                    |                                  |
 * | std::ranges::viewable_range      | *required*                         | *guaranteed*                     |
 * | std::ranges::view                |                                    | *guaranteed*                     |
 * | std::ranges::sized_range         |                                    | *lost*                           |
 * | std::ranges::common_range        |                                    | *lost*                           |
 * | std::ranges::output_range        |                                    | *lost*                           |
 * | seqan3::const_iterable_range     |                                    | *preserved*                      |
 * |                                  |                                    |                                  |
 * | std::ranges::range_reference_t   | std::totally_ordered               | std::totally_ordered             |
 *
 * See the \link views views submodule documentation \endlink for detailed descriptions of the view properties.
 *
 * \hideinitializer
 *
 * \stableapi{Since version 3.1.}
 */
inline constexpr auto minimiser = detail::minimiser_fn{};

} // namespace seqan3::views
