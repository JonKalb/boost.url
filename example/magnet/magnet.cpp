//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

//[example_magnet

/*
    This example parses a magnet link into a new
    view type and prints its components to
    standard output.
*/


#include <boost/url/url_view.hpp>
#include <boost/url/optional.hpp>
#include <boost/url/rfc/absolute_uri_rule.hpp>
#include <boost/url/grammar/digit_chars.hpp>
#include <boost/url/grammar/parse.hpp>
#include "filtered_view.hpp"
#include <iostream>

namespace urls = boost::urls;

/// Callable to identify a magnet "exact topic"
/**
    This callable evaluates if a query parameter
    represents a magnet "exact topic".

    This callable is used as a filter for
    the topics_view.
 */
struct is_exact_topic {
    bool
    operator()(urls::query_param_view p);
};

/// Callable to identify a magnet url parameter
/**
    This callable evaluates if a query parameter
    has a given key and a url as its value.

    These urls are percent-encoded twice,
    which means we need to decode it once
    before attempting to parse it.

    This callable is used as a filter for
    the keys_view.
 */
template <class MutableString>
class is_url_with_key {
    urls::string_view k_;
    MutableString& buf_;
public:
    explicit
    is_url_with_key(
        urls::string_view key,
        MutableString& buffer)
        : k_(key)
        , buf_(buffer) {}

    bool
    operator()(urls::query_param_view p);
};

/// Callable to convert param values to urls
/**
    This callable converts the value of a
    query parameter into a urls::url_view.

    This callable is used as a transform
    function for the topics_view.
 */
struct to_url {
    urls::url_view
    operator()(urls::query_param_view p);
};

/// Callable to convert param values to urls::pct_encoded_view
/**
    This callable converts the value of a
    query parameter into a urls::pct_encoded_view.

    This callable is used as a transform
    function for the keys_view.
 */
struct to_decoded_value {
    urls::pct_encoded_view
    operator()(urls::query_param_view p)
    {
        return p.value;
    }
};

/// Callable to convert param values to info_hashes
/**
    This callable converts the value of a
    query parameter into a urls::string_view with
    its infohash.

    The infohash hash is a parameter of an
    exact topic field in the magnet link.

    This callable is used as a transform
    function for the info_hashes_view.
 */
struct to_infohash {
    urls::string_view
    operator()(urls::query_param_view p);
};

/// Callable to convert param values to protocols
/**
    This callable converts the value of a
    query parameter into a urls::string_view with
    its protocol.

    The protocol is a parameter of an exact
    topic field in the magnet link.

    This callable is used as a transform
    function for the protocols_view.
 */
struct to_protocol {
    urls::string_view
    operator()(urls::query_param_view p);
};

struct magnet_link_rule_t;

/// A new url type for magnet links
/**
    This class represents a reference to a
    magnet link.

    Unlike a urls::url_view, which only represents the
    general syntax of urls, a magnet_link_view
    represents a reference to fields that are
    relevant to magnet links, while ignoring
    elements of the general syntax
    that are not relevant to the scheme.

    This allows us to use the general syntax
    parsers to create a representation that
    is more appropriate for the specified scheme
    syntax.

    @par Specification
    @li <a href="https://www.bittorrent.org/beps/bep_0005.html"
        >DHT Protocol</a>
    @li <a href="https://www.bittorrent.org/beps/bep_0009.html"
        >Extension for Peers to Send Metadata Files</a>
    @li <a href="https://www.bittorrent.org/beps/bep_0053.html"
        >Magnet URI extension</a>
    @li <a href="https://en.wikipedia.org/wiki/Magnet_URI_scheme"
        >Magnet URI scheme</a>

    @par References
    @li <a href="https://github.com/webtorrent/magnet-uri"
        >magnet-uri</a>

 */
class magnet_link_view
{
    urls::url_view u_;

public:
    /// A view of all exact topics in the magnet_link
    using topics_view =
        filtered_view<
            urls::params_view,
            urls::url_view,
            is_exact_topic,
            to_url>;

    /// A view of all info_hashes in the magnet_link
    using info_hashes_view =
        filtered_view<
            urls::params_view,
            urls::string_view,
            is_exact_topic,
            to_infohash>;

    /// A view of all protocols in the magnet_link
    using protocols_view =
        filtered_view<
            urls::params_view,
            urls::string_view,
            is_exact_topic,
            to_protocol>;

    /// A view of all urls with the specified key in the magnet_link
    /**
        A number of fields in a magnet link refer
        to a list of urls with the same query
        parameter keys.
     */
    template <class MutableString>
    using keys_view =
        filtered_view<
            urls::params_view,
            urls::pct_encoded_view,
            is_url_with_key<MutableString>,
            to_decoded_value>;

    /// URNs to the file or files hashes
    /**
       An exact topic is the main field of a
       magnet link. A magnet link must contain
       one or more exact topics with the query
       key "xt" or ["xt.1", "xt.2", ...].

       The value of each exact topic is a URN
       representing the file hash and the protocol
       to access the file.

       @return A view of all exact topic URNs in the link
     */
    topics_view
    exact_topics() const noexcept;

    /// Info hash of the file or files
    /**
       @return A view of all info hashes in exact topics
     */
    info_hashes_view
    info_hashes() const noexcept;

    /// Protocol of the exact topics
    /**
       @return A view of all protocols in exact topics
     */
    protocols_view
    protocols() const noexcept;

    /// Return view of address trackers
    /**
        A tracker URL is used to obtain resources
        for BitTorrent downloads.

        @param buffer Temporary buffer to decode
        a url returned by the view.

        @return A view of all address trackers in the link
     */
    template <class MutableString>
    keys_view<MutableString>
    address_trackers(MutableString& buffer) const;

    /// Return view of exact sources
    /**
        An exact source URL is a direct download
        link to the file.

        @param buffer Temporary buffer to decode
        a url returned by the view.

        @return A view of all exact sources
     */
    template <class MutableString>
    keys_view<MutableString>
    exact_sources(MutableString& buffer) const;

    /// Return view of acceptable sources
    /**
        An acceptable source URL is a direct
        download link to the file that can be
        used as a fallback for exact sources.

        @param buffer Temporary buffer to decode
        a url returned by the view.

        @return A view of all acceptable sources
     */
    template <class MutableString>
    keys_view<MutableString>
    acceptable_sources(MutableString& buffer) const;

    /// Return keyword topic
    /**
        The keyword topic is the search keywords
        to use in P2P networks.

        @par Example
        kt=martin+luther+king+mp3

        @return Keyword topic
     */
    urls::optional<urls::pct_encoded_view>
    keyword_topic() const noexcept;

    /// Return manifest topics
    /**
        This function returns a link to the
        metafile that contains a list of magneto.

        @par Specification
        @li <a href="http://rakjar.de/gnuticles/MAGMA-Specsv22.txt"
            >MAGnet MAnifest</a>

        @param buffer Temporary buffer to decode
        a url returned by the view.

        @return A view of manifest topics
     */
    template <class MutableString>
    keys_view<MutableString>
    manifest_topics(MutableString& buffer) const;

    /// Return display name
    /**
        This function returns a filename to
        display to the user. This field is
        only used for convenience.

        @par Specification
        @li <a href="http://rakjar.de/gnuticles/MAGMA-Specsv22.txt"
            >MAGnet MAnifest</a>

        @return Display name
     */
    urls::optional<urls::pct_encoded_view>
    display_name() const noexcept;

    // The payload data served over HTTP(S)

    /// Return web seed
    /**
        The web seed represents the payload data
        served over HTTP(S).

        @param buffer Temporary buffer to decode
        a url returned by the view.

        @return Web seed
     */
    template <class MutableString>
    keys_view<MutableString>
    web_seed(MutableString& buffer) const;

    /// Return extra supplement parameter
    /**
        This function returns informal options
        and parameters of the magnet link.

        Query parameters whose keys have the
        prefix "x." are used in magnet links
        for extra parameters. These names
        are guaranteed to never be standardized.

        @par Example
        x.parameter_name=parameter_data

        @return Web seed
     */
    urls::optional<urls::pct_encoded_view>
    param(urls::string_view key) const noexcept;

    friend
    std::ostream&
    operator<<(std::ostream& os, magnet_link_view m)
    {
        return os << m.u_;
    }

private:
    // get a query parameter as a urls::pct_encoded_view
    urls::optional<urls::pct_encoded_view>
    decoded_param(urls::string_view key) const noexcept;

    // get a query parameter as a urls::url_view
    urls::optional<urls::url_view>
    url_param(urls::string_view key) const noexcept;

    friend magnet_link_rule_t;
};

bool
is_exact_topic::
operator()(urls::query_param_view p)
{
    // These comparisons use the lazy
    // operator== for urls::pct_encoded_view
    // For instance, the comparison also works
    // if the underlying key is "%78%74"/
    if (p.key == "xt")
        return true;
    return
        p.key.size() > 3 &&
        *std::next(p.key.begin(), 0) == 'x' &&
        *std::next(p.key.begin(), 1) == 't' &&
        *std::next(p.key.begin(), 2) == '.' &&
        std::all_of(
            std::next(p.key.begin(), 3),
            p.key.end(),
            urls::grammar::digit_chars);
}

template <class MutableString>
bool
is_url_with_key<MutableString>::
operator()(urls::query_param_view p)
{
    if (p.key != k_)
        return false;
    urls::error_code ec;
    p.value.assign_to(buf_);
    if (ec.failed())
        return false;
    urls::result<urls::url_view> r =
        urls::parse_uri(buf_);
    return r.has_value();
}

urls::url_view
to_url::
operator()(urls::query_param_view p)
{
    // `to_url` is used in topics_view,
    // where the URL is not
    // percent-encoded twice.
    // Thus, we can already parse the
    // encoded value.
    return
        urls::parse_uri(p.value.encoded()).value();
}

urls::string_view
to_infohash::
operator()(urls::query_param_view p)
{
    urls::url_view topic =
        urls::parse_uri(p.value.encoded()).value();
    urls::string_view t = topic.encoded_path();
    std::size_t pos = t.find_last_of(':');
    if (pos != urls::string_view::npos)
        return t.substr(pos + 1);
    return t;
}

urls::string_view
to_protocol::
operator()(urls::query_param_view p)
{
    urls::url_view topic =

        urls::parse_uri(p.value.encoded()).value();
    urls::string_view t = topic.encoded_path();
    std::size_t pos = t.find_last_of(':');
    return t.substr(0, pos);
}

auto
magnet_link_view::exact_topics() const noexcept
    -> topics_view
{
    return {u_.params()};
}

auto
magnet_link_view::info_hashes() const noexcept
    -> info_hashes_view
{
    return {u_.params()};
}

auto
magnet_link_view::protocols() const noexcept
    -> protocols_view
{
    return {u_.params()};
}

template <class MutableString>
auto
magnet_link_view::address_trackers(MutableString& buffer) const
    -> keys_view<MutableString>
{
    return {
        u_.params(),
        is_url_with_key<MutableString>{"tr", buffer}};
}

template <class MutableString>
auto
magnet_link_view::exact_sources(MutableString& buffer) const
    -> keys_view<MutableString>
{
    return {
        u_.params(),
        is_url_with_key<MutableString>{"xs", buffer}};
}

template <class MutableString>
auto
magnet_link_view::acceptable_sources(MutableString& buffer) const
    -> keys_view<MutableString>
{
    return {
        u_.params(),
        is_url_with_key<MutableString>{"as", buffer}};
}

urls::optional<urls::pct_encoded_view>
magnet_link_view::keyword_topic() const noexcept
{
    return decoded_param("kt");
}

template <class MutableString>
auto
magnet_link_view::manifest_topics(MutableString& buffer) const
    -> keys_view<MutableString>
{
    return {
        u_.params(),
        is_url_with_key<MutableString>{"mt", buffer}};
}

urls::optional<urls::pct_encoded_view>
magnet_link_view::display_name() const noexcept
{
    return decoded_param("dn");
}

template <class MutableString>
auto
magnet_link_view::web_seed(MutableString& buffer) const
    -> keys_view<MutableString>
{
    return {
        u_.params(),
        is_url_with_key<MutableString>{"ws", buffer}};
}

urls::optional<urls::pct_encoded_view>
magnet_link_view::param(urls::string_view key) const noexcept
{
    urls::params_view ps = u_.params();
    auto it = ps.begin();
    auto end = ps.end();
    while (it != end)
    {
        urls::query_param_view p = *it;
        if (p.key.size() < 2)
        {
            ++it;
            continue;
        }
        auto first = p.key.begin();
        auto mid = std::next(p.key.begin(), 2);
        auto last = p.key.end();
        urls::pct_encoded_view prefix(
            urls::string_view(first.base(), mid.base()));
        urls::pct_encoded_view suffix(
            urls::string_view(mid.base(), last.base()));
        if (prefix == "x." &&
            suffix == key &&
            p.has_value)
            return urls::pct_encoded_view(p.value);
        ++it;
    }
    return boost::none;
}

urls::optional<urls::pct_encoded_view>
magnet_link_view::decoded_param(urls::string_view key) const noexcept
{
    urls::params_encoded_view ps = u_.encoded_params();
    auto it = ps.find(key);
    if (it != ps.end() && (*it).has_value)
        return urls::pct_encoded_view((*it).value);
    return boost::none;
}

urls::optional<urls::url_view>
magnet_link_view::url_param(urls::string_view key) const noexcept
{
    urls::params_encoded_view ps = u_.encoded_params();
    auto it = ps.find(key);
    if (it != ps.end() && (*it).has_value)
    {
        urls::result<urls::url_view> r =
            urls::parse_uri((*it).value);
        if (r)
            return *r;
    }
    return boost::none;
}

/** Rule to match a magnet link
*/
struct magnet_link_rule_t
{
    /// Value type returned by the rule
    using value_type = magnet_link_view;

    /// Parse a sequence of characters into a magnet_link_view
    urls::result< value_type >
    parse( char const*& it, char const* end ) const noexcept;
};

auto
magnet_link_rule_t::parse(
    char const*& it,
    char const* end ) const noexcept
    -> urls::result< value_type >
{
    // 1) Parse url with the general uri syntax
    urls::result<urls::url_view> r =
        urls::grammar::parse(it, end, urls::absolute_uri_rule);
    if(!r)
        return urls::grammar::error::invalid;
    magnet_link_view m;
    m.u_ = *r;

    // 2) Check if exact topics are valid urls
    // and that we have at least one. This is the
    // only mandatory field in magnet links.
    auto ps = m.u_.params();
    auto pit = ps.begin();
    auto pend = ps.end();
    pit = std::find_if(pit, pend, is_exact_topic{});
    if (pit == pend)
    {
        // no exact topic in the magnet link
        return urls::grammar::error::invalid;
    }

    // all topics should parse as valid urls
    if (!std::all_of(pit, pend, [](
        urls::query_param_view p)
    {
        if (!is_exact_topic{}(p))
            return true;
        urls::result<urls::url_view> u =
            urls::parse_uri(p.value.encoded());
        return u.has_value();
    }))
        return urls::grammar::error::invalid;

    // all other fields are optional
    // magnet link is OK
    return m;
}

constexpr magnet_link_rule_t magnet_link_rule{};

/** Return a parsed magnet link from a string, or error.

    This is a more convenient user-facing function
    to parse magnet links.
*/
urls::result< magnet_link_view >
parse_magnet_link( urls::string_view s ) noexcept
{
    return urls::grammar::parse(s, magnet_link_rule);
}

int main(int argc, char** argv)
{
    // This example shows how to use custom parsing
    // to process alternate URI schemes, in this
    // case "magnet"
    if (argc != 2) {
        std::cout << argv[0] << "\n";
        std::cout << "magnet <link>\n"
                     "example: magnet magnet:?xt=urn:btih:d2474e86c95b19b8bcfdb92bc12c9d44667cfa36"
                                            "&dn=Leaves+of+Grass+by+Walt+Whitman.epub"
                                            "&tr=udp%3A%2F%2Ftracker.example4.com%3A80"
                                            "&tr=udp%3A%2F%2Ftracker.example5.com%3A80"
                                            "&tr=udp%3A%2F%2Ftracker.example3.com%3A6969"
                                            "&tr=udp%3A%2F%2Ftracker.example2.com%3A80"
                                            "&tr=udp%3A%2F%2Ftracker.example1.com%3A1337\n";
        return EXIT_FAILURE;
    }

    urls::result<magnet_link_view> r =
        parse_magnet_link(argv[1]);
    if (!r)
        return EXIT_FAILURE;

    magnet_link_view m = *r;
    std::cout << "link: " << m << "\n";

    auto xt = m.exact_topics();
    for (auto h : xt)
        std::cout << "topic: " << h << "\n";

    auto hs = m.info_hashes();
    for (auto h : hs)
        std::cout << "hash: " << h << "\n";

    auto ps = m.protocols();
    for (auto p : ps)
        std::cout << "protocol: " << p << "\n";

    std::string buffer;
    auto tr = m.address_trackers(buffer);
    for (auto h : tr)
        std::cout << "tracker: " << h << "\n";

    auto xs = m.exact_sources(buffer);
    for (auto x : xs)
        std::cout << "exact source: " << x << "\n";

    auto as = m.acceptable_sources(buffer);
    for (auto a : as)
        std::cout << "topic: " << a << "\n";

    auto mt = m.manifest_topics(buffer);
    for (auto a : mt)
        std::cout << "manifest topic: " << a << "\n";

    auto ws = m.web_seed(buffer);
    for (auto a : ws)
        std::cout << "web seed: " << a << "\n";

    auto kt = m.keyword_topic();
    if (kt)
        std::cout << "keyword topic: " << *kt << "\n";

    auto dn = m.display_name();
    if (dn)
        std::cout << "display name: " << *dn << "\n";

    return EXIT_SUCCESS;
}

//]
