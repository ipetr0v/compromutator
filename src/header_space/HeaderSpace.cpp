#include "HeaderSpace.hpp"

int get_len(const char* str) {
    auto commas = (bool)strchr(str, ',');
    int div = CHAR_BIT + commas;
    int len = (int)strlen(str) + commas;
    assert(len % div == 0);
    len /= div;
    return len;
}

HeaderSpace::HeaderSpace(std::string str):
    length_(get_len(str.c_str()))
{
    assert(length_ > 0);
    hs_ = hs_create(length_);
    hs_add(hs_, array_from_str(str.c_str()));
}

HeaderSpace::HeaderSpace(const HeaderSpace& other):
    length_(other.length_)
{
    hs_ = hs_create(other.length_);
    hs_copy(hs_, other.hs_);
}

HeaderSpace::HeaderSpace(HeaderSpace&& other) noexcept:
    length_(other.length_), hs_(other.hs_)
{
    other.hs_ = nullptr;
}

HeaderSpace::HeaderSpace(int length):
    length_(length)
{
    hs_ = hs_create(length_);
}

HeaderSpace::HeaderSpace(struct hs* hs, int length):
    length_(length)
{
    hs_ = hs_create(length_);
    if (hs) hs_copy(hs_, hs);
}

HeaderSpace HeaderSpace::emptySpace(int length)
{
    return HeaderSpace(length);
}

HeaderSpace HeaderSpace::wholeSpace(int length)
{
    HeaderSpace header(length);
    array_t* whole_space = array_create(length, BIT_X);
    hs_add(header.hs_, whole_space);
    return header;
}

HeaderSpace::~HeaderSpace()
{
    this->clear();
}

void HeaderSpace::clear()
{
    if (hs_) hs_free(hs_);
}

HeaderSpace& HeaderSpace::operator=(const HeaderSpace& other)
{
    length_ = other.length_;
    hs_ = hs_create(other.length_);
    hs_copy(hs_, other.hs_);
    return *this;
}

HeaderSpace& HeaderSpace::operator=(HeaderSpace&& other) noexcept
{
    length_ = other.length_;
    hs_ = other.hs_;
    other.hs_ = nullptr;
    return *this;
}

bool HeaderSpace::operator==(const HeaderSpace &other) const
{
    assert(length_ == other.length_);
    return (*this - other).empty() && (other - *this).empty();
    // TODO: maybe compare it in another way?
    //hs_compact(hs_);
    //struct hs* other_hs = hs_copy_a(other.hs_);
    //bool is_equal = array_is_eq(, , length_);
    //hs_free(other_hs);
    //return is_equal;
}

bool HeaderSpace::operator!=(const HeaderSpace &other) const
{
    return !(*this == other);
}

HeaderSpace& HeaderSpace::operator~()
{
    hs_cmpl(hs_);
    return *this;
}

HeaderSpace& HeaderSpace::operator+=(const HeaderSpace& right) {
    hs_sum(hs_, right.hs_);
    return *this;
}

HeaderSpace& HeaderSpace::operator&=(const HeaderSpace& right)
{
    struct hs* intersection = hs_isect_a(hs_, right.hs_);
    if (intersection) {
        hs_copy(hs_, intersection);
    }
    else {
        this->clear();
        hs_ = hs_create(length_);
    }
    return *this;
}

HeaderSpace& HeaderSpace::operator-=(const HeaderSpace& right)
{
    hs_minus(hs_, right.hs_);
    return *this;
}

HeaderSpace HeaderSpace::operator+(const HeaderSpace& right) const
{
    HeaderSpace header(hs_, length_);
    header += right;
    return header;
}

HeaderSpace HeaderSpace::operator&(const HeaderSpace& right) const
{
    return HeaderSpace(hs_isect_a(hs_, right.hs_), length_);
}

HeaderSpace HeaderSpace::operator-(const HeaderSpace& right) const
{
    HeaderSpace header(hs_, length_);
    header -= right;
    return header;
}

std::ostream& operator<<(std::ostream& os, const HeaderSpace& header)
{
    struct hs* output_hs = hs_copy_a(header.hs_);
    //hs_compact(output_hs);

    char* output_string = hs_to_str(output_hs);
    os << output_string;

    free(output_string);
    hs_free(output_hs);
    return os;
}

HeaderChanger::HeaderChanger(int length):
    length_(length), identity_(true)
{
    // Function without rewrite
    mask_            = array_create(length_, BIT_1);
    rewrite_         = array_create(length_, BIT_0);
    inverse_rewrite_ = array_create(length_, BIT_0);
}

HeaderChanger::HeaderChanger(const HeaderChanger& other):
    length_(other.length_), identity_(other.identity_)
{
    mask_            = array_copy(other.mask_, length_);
    rewrite_         = array_copy(other.rewrite_, length_);
    inverse_rewrite_ = array_copy(other.inverse_rewrite_, length_);
}

HeaderChanger::HeaderChanger(HeaderChanger&& other) noexcept:
    length_(other.length_),  identity_(other.identity_)
{
    mask_            = other.mask_;
    rewrite_         = other.rewrite_;
    inverse_rewrite_ = other.inverse_rewrite_;

    other.mask_            = nullptr;
    other.rewrite_         = nullptr;
    other.inverse_rewrite_ = nullptr;
}

HeaderChanger::HeaderChanger(const char* transfer_str):
    length_(get_len(transfer_str))
{
    assert(length_ > 0);
    array_t* transfer_array = array_from_str(transfer_str);

    mask_            = array_create(length_, BIT_1);
    rewrite_         = array_create(length_, BIT_0);
    inverse_rewrite_ = array_create(length_, BIT_0);

    bool identity_check = true;
    for (int i = 0; i < length_*CHAR_BIT; i++) {
        int byte = i/CHAR_BIT;
        int bit = i%CHAR_BIT;
        
        enum bit_val transfer_bit = array_get_bit(transfer_array, byte, bit);
        enum bit_val mask_bit;
        enum bit_val rewrite_bit;
        enum bit_val inverse_rewrite_bit;
        
        switch(transfer_bit) {
        case BIT_0:
        case BIT_1:
        case BIT_X:
            mask_bit = BIT_0;
            rewrite_bit = transfer_bit;
            inverse_rewrite_bit = BIT_X;
            identity_check = false;
            break;
        
        case BIT_Z:
        default:
            mask_bit = BIT_1;
            rewrite_bit = BIT_0;
            inverse_rewrite_bit = BIT_0;
            break;
        }
        assert(mask_bit != BIT_Z and rewrite_bit != BIT_Z and
               inverse_rewrite_bit != BIT_Z);
        
        array_set_bit(mask_, mask_bit, byte, bit);
        array_set_bit(rewrite_, rewrite_bit, byte, bit);
        array_set_bit(inverse_rewrite_, inverse_rewrite_bit, byte, bit);
    }
    identity_ = identity_check;
    array_free(transfer_array);
}

HeaderChanger::HeaderChanger(const char* mask_str, const char* rewrite_str)
{
    int mask_len = get_len(mask_str);
    int rewrite_len = get_len(rewrite_str);
    assert(mask_len == rewrite_len);
    length_ = mask_len;

    mask_            = array_from_str(mask_str);
    rewrite_         = array_from_str(rewrite_str);
    inverse_rewrite_ = array_create(length_, BIT_0);

    bool identity_check = true;
    for (int i = 0; i < length_*CHAR_BIT; i++) {
        int byte = i/CHAR_BIT;
        int bit = i%CHAR_BIT;

        enum bit_val transfer_bit;
        enum bit_val mask_bit = array_get_bit(mask_, byte, bit);
        enum bit_val rewrite_bit = array_get_bit(rewrite_, byte, bit);
        assert(mask_bit == BIT_Z || rewrite_bit == BIT_Z);
        enum bit_val inverse_rewrite_bit;

        if(mask_bit == BIT_1) {
            transfer_bit = BIT_Z;
            inverse_rewrite_bit = BIT_0;
        }
        else {
            transfer_bit = rewrite_bit;
            inverse_rewrite_bit = BIT_X;
        }
        assert(mask_bit != BIT_Z and rewrite_bit != BIT_Z and
               inverse_rewrite_bit != BIT_Z);

        if (transfer_bit != BIT_Z) {
            identity_check = false;
        }

        array_set_bit(inverse_rewrite_, inverse_rewrite_bit, byte, bit);
    }
    identity_ = identity_check;
}

HeaderChanger::~HeaderChanger()
{
    this->clear();
}

void HeaderChanger::clear()
{
    if (mask_)            array_free(mask_);
    if (rewrite_)         array_free(rewrite_);
    if (inverse_rewrite_) array_free(inverse_rewrite_);
}

HeaderChanger HeaderChanger::identityHeaderChanger(int length)
{
    return HeaderChanger(length);
}

HeaderChanger& HeaderChanger::operator=(const HeaderChanger& other)
{
    this->clear();

    length_ = other.length_;
    identity_ = other.identity_;

    mask_            = array_copy(other.mask_, length_);
    rewrite_         = array_copy(other.rewrite_, length_);
    inverse_rewrite_ = array_copy(other.inverse_rewrite_, length_);
    return *this;
}

HeaderChanger& HeaderChanger::operator=(HeaderChanger&& other) noexcept
{
    this->clear();

    length_ = other.length_;
    identity_ = other.identity_;

    mask_            = other.mask_;
    rewrite_         = other.rewrite_;
    inverse_rewrite_ = other.inverse_rewrite_;

    other.mask_            = nullptr;
    other.rewrite_         = nullptr;
    other.inverse_rewrite_ = nullptr;
    return *this;
}

bool HeaderChanger::operator==(const HeaderChanger &other) const
{
    return array_is_eq(mask_, other.mask_, length_) &&
           array_is_eq(rewrite_, other.rewrite_, length_) &&
           array_is_eq(inverse_rewrite_, other.inverse_rewrite_, length_);
}

bool HeaderChanger::operator!=(const HeaderChanger &other) const
{
    return !(*this == other);
}

HeaderChanger HeaderChanger::operator*=(const HeaderChanger& right)
{
    assert(length_ == right.length_);
    identity_ &= right.identity_;
    if (not right.identity_) {
        // New mask is a logical and
        for (int i = 0; i < length_*CHAR_BIT; i++) {
            int byte = i/CHAR_BIT;
            int bit = i%CHAR_BIT;

            enum bit_val mask_bit = array_get_bit(mask_, byte, bit);
            enum bit_val right_mask_bit = array_get_bit(right.mask_, byte, bit);
            assert(mask_bit != BIT_Z and right_mask_bit != BIT_Z);
            enum bit_val result_mask_bit;

            if(mask_bit == BIT_0 || right_mask_bit == BIT_0) {
                result_mask_bit = BIT_0;
            }
            else {
                result_mask_bit = BIT_1;
            }

            array_set_bit(mask_, result_mask_bit, byte, bit);
        }

        array_rewrite(rewrite_, right.mask_, right.rewrite_, length_);
        array_rewrite(inverse_rewrite_, right.mask_,
                      right.inverse_rewrite_, length_);
    }
    return *this;
}

HeaderSpace HeaderChanger::apply(const HeaderSpace& header) const
{
    HeaderSpace new_header(header);
    if (not identity_) {
        hs_rewrite(new_header.hs_, mask_, rewrite_);
    }
    return std::move(new_header);
}

HeaderSpace HeaderChanger::inverse(const HeaderSpace& header) const
{
    HeaderSpace new_header(header);
    if (not identity_) {
        hs_rewrite(new_header.hs_, mask_, inverse_rewrite_);
    }
    return std::move(new_header);
}

std::ostream& operator<<(std::ostream& os, const HeaderChanger& transfer)
{
    // Create transfer string representation
    int length = transfer.length_;
    array_t* transfer_array = array_create(length, BIT_Z);
    for (int i = 0; i < length*CHAR_BIT; i++) {
        int byte = i/CHAR_BIT;
        int bit = i%CHAR_BIT;

        enum bit_val transfer_bit;
        enum bit_val mask_bit = array_get_bit(transfer.mask_, byte, bit);
        enum bit_val rewrite_bit = array_get_bit(transfer.rewrite_, byte, bit);

        if(mask_bit == BIT_1) {
            transfer_bit = BIT_Z;
        }
        else {
            transfer_bit = rewrite_bit;
        }
        array_set_bit(transfer_array, transfer_bit, byte, bit);
    }

    os << array_to_str(transfer_array, length, false);
    array_free(transfer_array);
    return os;
}
