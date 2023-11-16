#pragma once

#include <fstream>
#include <streambuf>

class IgnoreLineCommentsBuf : public std::streambuf
{
public:
    using Base = std::streambuf;
    using Base::char_type;
    using Base::int_type;
    using Base::traits_type;

    explicit IgnoreLineCommentsBuf(std::streambuf* buf)
    : m_src(buf)
    {
        this->setg(&m_ch, &m_ch + 1, &m_ch + 1);
    }

    IgnoreLineCommentsBuf(const IgnoreLineCommentsBuf&) = delete;
    IgnoreLineCommentsBuf(IgnoreLineCommentsBuf&&)      = delete;

    IgnoreLineCommentsBuf operator=(const IgnoreLineCommentsBuf&) = delete;
    IgnoreLineCommentsBuf operator=(IgnoreLineCommentsBuf&&)      = delete;

protected:
    int_type underflow() override
    {
        traits_type::int_type i = m_src->sbumpc();
        if (!traits_type::eq_int_type(i, traits_type::eof())) {
            m_ch = traits_type::to_char_type(i);
            if (m_ch == '#') {
                i = eat_comment();
            }
            this->setg(&m_ch, &m_ch, &m_ch + 1);
            return i;
        }

        return i;
    }

private:
    int_type eat_comment()
    {
        traits_type::int_type i = m_src->sbumpc();
        while (!traits_type::eq_int_type(i, '\n') && i != traits_type::eof()) {
            i = m_src->sbumpc();
        }
        return i;
    }

    char            m_ch;
    std::streambuf* m_src;
};

class IgnoreLineCommentsStream : public std::ifstream
{
public:
    explicit IgnoreLineCommentsStream(const std::filesystem::path& file_name,
                                      std::ios_base::openmode      mode = std::ios_base::in)
    : std::ifstream(file_name)
    , m_buf(this->rdbuf())
    {
        // std::basic_fstream does not expose the setting version of rdbuf. We have to subvert the system and use the
        // protected set_rdbuf call.
        this->set_rdbuf(std::addressof(m_buf));
    }

private:
    IgnoreLineCommentsBuf m_buf;
};
