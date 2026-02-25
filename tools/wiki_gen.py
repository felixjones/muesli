#!/usr/bin/env python3
# AI Generated file! TODO: Replace with human code
"""
GitHub Wiki generator for C++ library API documentation.
Extracts comprehensive header documentation with member functions, types, and examples.
Generates structured markdown suitable for GitHub Wiki.
"""

import re
from pathlib import Path
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple
from collections import defaultdict

@dataclass
class DoxyComment:
    """Parsed doxygen documentation block."""
    brief: str
    detailed: str
    category: str  # @category tag
    template_params: Dict[str, str]
    member_params: Dict[str, str]  # @param for function parameters
    returns: str
    example: str
    notes: str
    see_also: List[str]

@dataclass
class MemberType:
    """A member type (typedef, using)."""
    kind: str  # 'typedef', 'using'
    name: str
    signature: str
    doxy: Optional[DoxyComment]

@dataclass
class MemberConstant:
    """A static member constant."""
    name: str
    type_str: str
    value: Optional[str]
    doxy: Optional[DoxyComment]

@dataclass
class MemberFunction:
    """A member function."""
    name: str
    signature: str
    is_static: bool
    is_constexpr: bool
    doxy: Optional[DoxyComment]

@dataclass
class CppDecl:
    """A documented C++ struct/class."""
    kind: str  # 'struct', 'class'
    name: str
    template_params: List[str]
    signature: str
    doxy: Optional[DoxyComment]
    member_types: List[MemberType] = field(default_factory=list)
    member_constants: List[MemberConstant] = field(default_factory=list)
    member_functions: List[MemberFunction] = field(default_factory=list)
    line: int = 0

def parse_doxygen_block(text: str) -> Optional[DoxyComment]:
    """Parse comprehensive doxygen documentation with improved formatting."""
    # Remove /** and */ markers and extract just content
    text = text.strip()
    if not text.startswith('/*'):
        return None

    # Remove /* and */
    text = re.sub(r'/\*\*\s*|\s*\*/$', '', text)

    lines = text.split('\n')
    content_lines = []
    for line in lines:
        line = line.strip()
        # Remove leading * but preserve any indentation after it
        if line.startswith('*'):
            line = re.sub(r'^\*\s?', '', line)
        content_lines.append(line)

    content = '\n'.join(content_lines).strip()
    if not content:
        return None

    # Extract @category first
    category = ""
    category_match = re.search(r'@category\s+(.+?)(?=@|\Z)', content)
    if category_match:
        category = category_match.group(1).strip()
        # Remove @category from content so it doesn't appear in brief/detailed
        content = re.sub(r'@category\s+.+?(?=@|\Z)', '', content)

    # Split on first @ tag to separate documentation from tags
    parts = re.split(r'@', content, maxsplit=1)
    main_doc = parts[0].strip()
    rest = '@' + parts[1] if len(parts) > 1 else ""

    # Extract brief (first sentence)
    brief = ""

    # If main_doc is empty, brief might be in @brief tag
    if not main_doc and rest:
        brief_match = re.search(r'@brief\s+(.+?)(?=@|\Z)', rest, re.DOTALL)
        if brief_match:
            # Extract just the first sentence/line from @brief
            brief_text = brief_match.group(1).strip()
            # Get first line or first sentence
            first_line = brief_text.split('\n')[0].strip()
            brief = first_line
    elif main_doc:
        brief_match = re.match(r'^([^.\n]+\.?)', main_doc)
        if brief_match:
            brief = brief_match.group(1).strip()
        else:
            brief = main_doc.split('\n')[0].strip()

    # Extract detailed (everything in main_doc after brief, excluding code blocks)
    detailed = ""

    # If we used @brief tag, extract detailed from the rest of @brief content
    if not main_doc and rest:
        brief_match = re.search(r'@brief\s+(.+?)(?=@|\Z)', rest, re.DOTALL)
        if brief_match:
            brief_full = brief_match.group(1).strip()
            # Split into lines and find where detailed starts
            lines = brief_full.split('\n')

            # First non-empty line is brief (already extracted)
            # Look for subsequent paragraphs as detailed
            detailed_lines = []
            skip_first = True
            for line in lines:
                line = line.strip()
                if skip_first and line:
                    # Skip the first non-empty line (it's the brief)
                    skip_first = False
                    continue
                if not skip_first:
                    detailed_lines.append(line)

            if detailed_lines:
                detailed = '\n'.join(detailed_lines).strip()
    elif len(main_doc) > len(brief):
        remaining = main_doc[len(brief):].strip()
        # Remove code examples from detailed (they go to example section)
        remaining = re.sub(r'```[^`]*```', '', remaining, flags=re.DOTALL)
        remaining = re.sub(r'\n\n+', '\n\n', remaining)  # Clean up double newlines
        detailed = remaining.strip()

    # Extract @tparam
    template_params = {}
    for match in re.finditer(r'@tparam\s+(\w+)\s+(.+?)(?=@|\Z)', rest, re.DOTALL):
        name, desc = match.groups()
        # Remove code blocks from description
        desc = re.sub(r'```[^`]*```', '', desc, flags=re.DOTALL)
        desc = re.sub(r'@code.*?@endcode', '', desc, flags=re.DOTALL)
        # Stop at common markers that indicate this isn't a simple parameter description
        desc = re.split(r'\n\s*(?:Template member types:|Example usage:|Example:|Note:)', desc)[0]
        # Normalize whitespace and collapse to single line for tables
        desc = ' '.join(desc.split())
        # Limit to first sentence for table display
        desc = re.match(r'^([^.!?]*[.!?]?)', desc).group(1).strip() if desc else ""
        template_params[name.strip()] = desc.strip()

    # Extract @param (function parameters)
    member_params = {}
    for match in re.finditer(r'@param\s+(\w+)\s+(.+?)(?=@|\Z)', rest, re.DOTALL):
        name, desc = match.groups()
        # Remove code blocks from description
        desc = re.sub(r'```[^`]*```', '', desc, flags=re.DOTALL)
        desc = re.sub(r'@code.*?@endcode', '', desc, flags=re.DOTALL)
        # Normalize whitespace and collapse to single line
        desc = ' '.join(desc.split())
        # Limit to first sentence
        desc = re.match(r'^([^.!?]*[.!?]?)', desc).group(1).strip() if desc else ""
        member_params[name.strip()] = desc.strip()

    # Extract @return (stop at next @ tag only, not at code/example text)
    returns = ""
    ret_match = re.search(r'@returns?\s+([^@]+?)(?=@|\Z)', rest, re.DOTALL)
    if ret_match:
        ret_text = ret_match.group(1).strip()
        # Remove code blocks and Example/Note sections from returns (they're not part of return value description)
        ret_text = re.sub(r'Example:.*', '', ret_text, flags=re.DOTALL).strip()
        ret_text = re.sub(r'Note:.*', '', ret_text, flags=re.DOTALL).strip()
        ret_text = re.sub(r'```[^`]*```', '', ret_text, flags=re.DOTALL).strip()
        returns = ' '.join(ret_text.split()).strip()

    # Extract @note
    notes = ""
    notes_match = re.search(r'@note\s+(.+?)(?=@|\Z)', rest, re.DOTALL)
    if notes_match:
        notes = ' '.join(notes_match.group(1).split()).strip()

    # Extract @code...@endcode (preferred format)
    example = ""
    code_match = re.search(r'@code\s*\n(.+?)\n@endcode', rest, re.DOTALL)
    if code_match:
        example_text = code_match.group(1)
        # Clean doxygen comment markers while preserving indentation
        lines_list = example_text.split('\n')
        cleaned_lines = []

        for line in lines_list:
            # Remove the doxygen marker " * " or "*" at the beginning
            # This preserves all indentation in the actual code
            if line.lstrip().startswith('*'):
                # Line has a *, remove it and the space after it
                cleaned = re.sub(r'^\s*\*\s?', '', line)
                cleaned_lines.append(cleaned)
            else:
                # Line doesn't have a marker, keep as-is
                cleaned_lines.append(line)

        # Join and strip only trailing/leading whitespace from the block
        joined = '\n'.join(cleaned_lines)
        example = joined.lstrip('\n').rstrip()

    # Also look for ```cpp...``` code blocks in main doc or rest section
    if not example:
        code_block = re.search(r'```(?:cpp)?\s*\n(.+?)\n```', main_doc, re.DOTALL)
        if code_block:
            example = code_block.group(1).strip()

    # If still not found, check the rest section (after @tags)
    if not example:
        code_block = re.search(r'```(?:cpp)?\s*\n(.+?)\n```', rest, re.DOTALL)
        if code_block:
            example = code_block.group(1).strip()

    # Extract @see
    see_also = []
    for match in re.finditer(r'@see\s+(.+?)(?=@|\Z)', rest, re.DOTALL):
        see_also.append(match.group(1).strip())

    return DoxyComment(
        brief=brief,
        detailed=detailed,
        category=category,
        template_params=template_params,
        member_params=member_params,
        returns=returns,
        example=example,
        notes=notes,
        see_also=see_also
    )

def extract_struct_members(struct_text: str, struct_doxy: Optional[DoxyComment]) -> Tuple[List[MemberType], List[MemberConstant], List[MemberFunction]]:
    """Extract member types, constants, and functions from struct body."""
    member_types = []
    member_constants = []
    member_functions = []

    # Find the struct body (between { and final })
    body_match = re.search(r'\{(.*)\}', struct_text, re.DOTALL)
    if not body_match:
        return member_types, member_constants, member_functions

    body = body_match.group(1)

    # Process line by line, looking for doxygen comments followed by declarations
    lines = body.split('\n')
    i = 0
    while i < len(lines):
        line = lines[i].strip()

        # Check if this line starts a doxygen comment
        if line.startswith('/**'):
            # Collect the full doxygen block
            doxy_lines = [line]
            i += 1
            while i < len(lines) and '*/' not in lines[i-1]:
                doxy_lines.append(lines[i])
                i += 1

            doxy_text = '\n'.join(doxy_lines)
            doxy = parse_doxygen_block(doxy_text)

            # Now get the declaration after the doxygen
            if i < len(lines):
                # Skip empty lines
                while i < len(lines) and not lines[i].strip():
                    i += 1

                if i >= len(lines):
                    break

                # Collect the declaration (may span multiple lines)
                decl_lines = []

                # Check if it's a template
                is_template = 'template' in lines[i]
                if is_template:
                    decl_lines.append(lines[i])
                    i += 1
                    # Skip to next non-empty line
                    while i < len(lines) and not lines[i].strip():
                        i += 1

                # Now get the actual declaration
                if i < len(lines):
                    decl_lines.append(lines[i])
                    i += 1

                    # If it's a function, collect until we hit the opening brace or semicolon
                    if '(' in decl_lines[-1]:
                        # Collect parameters and const/noexcept specifiers
                        paren_depth = decl_lines[-1].count('(') - decl_lines[-1].count(')')
                        found_body_or_end = '{' in decl_lines[-1] or ';' in decl_lines[-1]

                        while i < len(lines) and not found_body_or_end:
                            line_text = lines[i]
                            decl_lines.append(line_text)
                            paren_depth += line_text.count('(') - line_text.count(')')

                            # Check if we've closed all parens and hit body or semicolon
                            if paren_depth == 0 and ('{' in line_text or ';' in line_text):
                                found_body_or_end = True
                            i += 1

                        # Skip function body if it has one (don't include in signature)
                        last_line = decl_lines[-1] if decl_lines else ""
                        if '{' in last_line:
                            # Remove everything from { onwards from last line
                            brace_idx = last_line.index('{')
                            decl_lines[-1] = last_line[:brace_idx]

                            # Skip rest of body
                            brace_depth = 1
                            while i < len(lines) and brace_depth > 0:
                                brace_depth += lines[i].count('{') - lines[i].count('}')
                                i += 1

                    # Now classify the declaration
                    full_decl = ' '.join(decl_lines).strip()
                    full_decl = re.sub(r'\s+', ' ', full_decl)  # Normalize whitespace

                    if full_decl.startswith(('typedef', 'using')):
                        # Member type
                        kind_match = re.match(r'(typedef|using)\s+', full_decl)
                        kind = kind_match.group(1) if kind_match else 'typedef'
                        if 'using' in full_decl:
                            name_match = re.search(r'using\s+(\w+)\s*=', full_decl)
                        else:
                            name_match = re.search(r'\b(\w+)\s*[;=]?\s*$', full_decl)
                        name = name_match.group(1) if name_match else "unknown"

                        # Remove body for signature
                        sig = re.sub(r'\{.*\}', '', full_decl, flags=re.DOTALL).rstrip(';').strip()

                        member_types.append(MemberType(
                            kind=kind,
                            name=name,
                            signature=sig,
                            doxy=doxy
                        ))

                    elif 'constexpr' in full_decl and '(' not in full_decl:
                        # Member constant
                        const_match = re.search(r'constexpr\s+(?:auto|(\w+(?:<[^>]*>)?(?:\s*\*|&)?))\s+(\w+)\s*=\s*([^;{]*)', full_decl)
                        if const_match:
                            type_str = const_match.group(1) or "auto"
                            name = const_match.group(2)
                            value = const_match.group(3).strip()

                            member_constants.append(MemberConstant(
                                name=name,
                                type_str=type_str.strip(),
                                value=value if value else None,
                                doxy=doxy
                            ))

                    elif '(' in full_decl and ')' in full_decl:
                        # Member function
                        func_name_matches = list(re.finditer(r'(\w+)\s*\(', full_decl))
                        func_name_match = func_name_matches[-1] if func_name_matches else None
                        if func_name_match:
                            name = func_name_match.group(1)
                            is_constexpr = 'constexpr' in full_decl
                            is_static = 'static' in full_decl

                            # Clean up signature - keep template prefix, drop body
                            sig = re.sub(r'\{.*', '', full_decl).rstrip(';').strip()
                            sig = re.sub(r'\s+', ' ', sig)

                            member_functions.append(MemberFunction(
                                name=name,
                                signature=sig,
                                is_static=is_static,
                                is_constexpr=is_constexpr,
                                doxy=doxy
                            ))
        else:
            i += 1

    return member_types, member_constants, member_functions

def convert_see_also_to_link(item: str) -> str:
    """Convert a @see item to a markdown link if it's a known type."""
    item = item.strip()

    # Common codec and type names that should be linked
    known_types = {
        'identity_codec', 'constant_codec', 'array_codec', 'vector_codec',
        'tuple_codec', 'variant_codec', 'optional_codec', 'nullable_codec',
        'transform_codec', 'projector_codec', 'member_codec', 'apply_codec',
        'constrained_codec', 'delimited_codec', 'range_codec', 'monostate_codec',
        # Concepts
        'Codec', 'HasValueType', 'HasNextCodec', 'CanEncodeRvalue', 'CanEncodeConstRef',
        'CanDecodeRvalue', 'CanDecodeConstRef', 'HasBoolConversion', 'HasHasValue',
        'HasOptionalValueCheck', 'HasProjectToEncoded',
        # Codecs constants
        'int_codec', 'int8_codec', 'int16_codec', 'int32_codec', 'int64_codec',
        'uint8_codec', 'uint16_codec', 'uint32_codec', 'uint64_codec',
        'float_codec', 'double_codec', 'bool_codec', 'char_codec',
        'string_codec', 'u8string_codec', 'u16string_codec', 'u32string_codec',
        'size_codec', 'ptrdiff_codec', 'nullptr_codec', 'byte_codec',
    }

    # Check if it's a known type
    for known_type in known_types:
        if known_type in item:
            # Determine the page name based on type
            if known_type.endswith('_codec') or known_type.endswith('_codecs'):
                page = f"API-muesli-{known_type}"
            elif known_type in ['Codec', 'HasValueType', 'HasNextCodec', 'CanEncodeRvalue',
                                'CanEncodeConstRef', 'CanDecodeRvalue', 'CanDecodeConstRef',
                                'HasBoolConversion', 'HasHasValue', 'HasOptionalValueCheck',
                                'HasProjectToEncoded']:
                page = "API-muesli-util-concepts"
            elif '_codec' in item:
                page = f"API-muesli-{known_type}"
            else:
                # For codec constants, find their header
                if known_type in ['int8_codec', 'int16_codec', 'int32_codec', 'int64_codec',
                                  'uint8_codec', 'uint16_codec', 'uint32_codec', 'uint64_codec']:
                    page = "API-muesli-stdint_codecs"
                elif known_type in ['size_codec', 'ptrdiff_codec', 'nullptr_codec', 'byte_codec']:
                    page = "API-muesli-stddef_codecs"
                elif known_type in ['int_codec', 'float_codec', 'double_codec', 'bool_codec', 'char_codec']:
                    page = "API-muesli-fundamental_codecs"
                elif 'string_codec' in known_type:
                    page = "API-muesli-string_codecs"
                else:
                    page = f"API-muesli-{known_type}"

            # Create markdown link
            return f"[{item}]({page}#{known_type})"

    # Return as-is if not a known type
    return item

def add_cppreference_links(text: str) -> str:
    """Add cppreference.com links to std:: type mentions in text."""
    if not text:
        return text

    # Map of std:: types to their cppreference URLs
    std_types = {
        'std::variant': 'https://en.cppreference.com/w/cpp/utility/variant',
        'std::optional': 'https://en.cppreference.com/w/cpp/utility/optional',
        'std::tuple': 'https://en.cppreference.com/w/cpp/utility/tuple',
        'std::pair': 'https://en.cppreference.com/w/cpp/utility/pair',
        'std::vector': 'https://en.cppreference.com/w/cpp/container/vector',
        'std::array': 'https://en.cppreference.com/w/cpp/container/array',
        'std::map': 'https://en.cppreference.com/w/cpp/container/map',
        'std::unordered_map': 'https://en.cppreference.com/w/cpp/container/unordered_map',
        'std::set': 'https://en.cppreference.com/w/cpp/container/set',
        'std::string': 'https://en.cppreference.com/w/cpp/string/basic_string',
        'std::u8string': 'https://en.cppreference.com/w/cpp/string/basic_string',
        'std::u16string': 'https://en.cppreference.com/w/cpp/string/basic_string',
        'std::u32string': 'https://en.cppreference.com/w/cpp/string/basic_string',
        'std::wstring': 'https://en.cppreference.com/w/cpp/string/basic_string',
        'std::shared_ptr': 'https://en.cppreference.com/w/cpp/memory/shared_ptr',
        'std::unique_ptr': 'https://en.cppreference.com/w/cpp/memory/unique_ptr',
        'std::weak_ptr': 'https://en.cppreference.com/w/cpp/memory/weak_ptr',
        'std::span': 'https://en.cppreference.com/w/cpp/container/span',
        'std::byte': 'https://en.cppreference.com/w/cpp/types/byte',
        'std::size_t': 'https://en.cppreference.com/w/cpp/types/size_t',
        'std::ptrdiff_t': 'https://en.cppreference.com/w/cpp/types/ptrdiff_t',
        'std::nullptr_t': 'https://en.cppreference.com/w/cpp/types/nullptr_t',
        'std::monostate': 'https://en.cppreference.com/w/cpp/utility/variant/monostate',
        'std::nullopt': 'https://en.cppreference.com/w/cpp/utility/optional/nullopt',
        'std::nullopt_t': 'https://en.cppreference.com/w/cpp/utility/optional/nullopt_t',
        'std::int8_t': 'https://en.cppreference.com/w/cpp/types/integer',
        'std::uint8_t': 'https://en.cppreference.com/w/cpp/types/integer',
        'std::int16_t': 'https://en.cppreference.com/w/cpp/types/integer',
        'std::uint16_t': 'https://en.cppreference.com/w/cpp/types/integer',
        'std::int32_t': 'https://en.cppreference.com/w/cpp/types/integer',
        'std::uint32_t': 'https://en.cppreference.com/w/cpp/types/integer',
        'std::int64_t': 'https://en.cppreference.com/w/cpp/types/integer',
        'std::uint64_t': 'https://en.cppreference.com/w/cpp/types/integer',
        'std::intmax_t': 'https://en.cppreference.com/w/cpp/types/integer',
        'std::uintmax_t': 'https://en.cppreference.com/w/cpp/types/integer',
        'std::intptr_t': 'https://en.cppreference.com/w/cpp/types/integer',
        'std::uintptr_t': 'https://en.cppreference.com/w/cpp/types/integer',
    }

    # Replace each std:: type with a markdown link (avoid replacing if already in a link or code block)
    result = text
    for std_type, url in std_types.items():
        # Only replace if not already in a markdown link or code fence
        # Use word boundaries to avoid partial matches
        pattern = r'(?<![`\[])\b' + re.escape(std_type) + r'\b(?![`\]])'
        replacement = f'[{std_type}]({url})'
        result = re.sub(pattern, replacement, result)

    return result

def extract_declarations(file_path: Path) -> List[CppDecl]:
    """Parse header and extract documented structs/classes/variables with full member details."""
    text = file_path.read_text(encoding='utf-8')
    declarations = []

    # Find all doxygen blocks followed by struct/class or constexpr variable
    doxy_pattern = r'/\*\*.*?\*/'

    for doxy_match in re.finditer(doxy_pattern, text, re.DOTALL):
        doxy_text = doxy_match.group(0)
        doxy = parse_doxygen_block(doxy_text)
        if not doxy:
            continue

        # Find the declaration after this doxygen block
        pos_after_doxy = doxy_match.end()
        remaining = text[pos_after_doxy:]

        # First try: Match struct/class with inheritance
        # Handle: template<...> (requires ...)? struct NAME : bases ... {
        # Note: Template parameters can have nested <>  (e.g., Allocator = std::allocator<T>)
        # So we need to match balanced angle brackets
        decl_match = None
        template_part = ""

        if remaining.strip().startswith(('template<', 'template <')):
            # Try to find matching > for template
            angle_depth = 0
            template_start = remaining.find('<')
            template_end = None

            for i, char in enumerate(remaining[template_start:]):
                if char == '<':
                    angle_depth += 1
                elif char == '>':
                    angle_depth -= 1
                    if angle_depth == 0:
                        template_end = template_start + i + 1
                        break

            if template_end:
                # Now match the rest after the template
                rest_pattern = r'\s*(requires\s*[^{]*)?\s*(struct|class)\s+(\w+)\s*([^{]*)\{'
                rest_text = remaining[template_end:]
                rest_match = re.match(rest_pattern, rest_text, re.DOTALL)

                if rest_match:
                    template_part = remaining[:template_end]
                    requires_clause = rest_match.group(1) or ""
                    kind = rest_match.group(2)
                    name = rest_match.group(3)
                    inheritance = rest_match.group(4).strip()

                    # Create a match-like object
                    class TemplateMatch:
                        def __init__(self, template_part, requires_clause, kind, name, inheritance, rest_match):
                            self.template_part = template_part
                            self.requires_clause = requires_clause
                            self.kind = kind
                            self.name = name
                            self.inheritance = inheritance
                            self.rest_match = rest_match
                            self.end_pos = template_end + rest_match.end()

                        def group(self, n):
                            if n == 0: return None
                            elif n == 1: return self.template_part
                            elif n == 2: return self.requires_clause
                            elif n == 3: return self.kind
                            elif n == 4: return self.name
                            elif n == 5: return self.inheritance
                            return None

                        def end(self):
                            return self.end_pos

                    decl_match = TemplateMatch(template_part, requires_clause, kind, name, inheritance, rest_match)
        else:
            # No template, try simpler pattern
            decl_pattern = r'\s*(requires\s*[^{]*)?\s*(struct|class)\s+(\w+)\s*([^{]*)\{'
            rest_match = re.match(decl_pattern, remaining, re.DOTALL)
            if rest_match:
                requires_clause = rest_match.group(1) or ""
                kind = rest_match.group(2)
                name = rest_match.group(3)
                inheritance = rest_match.group(4).strip()

                class SimpleMatch:
                    def __init__(self, requires_clause, kind, name, inheritance, match):
                        self.requires_clause = requires_clause
                        self.kind = kind
                        self.name = name
                        self.inheritance = inheritance
                        self.match = match

                    def group(self, n):
                        if n == 0: return None
                        elif n == 1: return ""
                        elif n == 2: return self.requires_clause
                        elif n == 3: return self.kind
                        elif n == 4: return self.name
                        elif n == 5: return self.inheritance
                        return None

                    def end(self):
                        return self.match.end()

                decl_match = SimpleMatch(requires_clause, kind, name, inheritance, rest_match)

        # If not a struct/class, try concept declaration
        if not decl_match:
            # Concepts have format: template<...> concept NAME = constraint;
            # Look for the 'concept' keyword as a word boundary match
            concept_pattern = r'(?:^|\s)concept\s+(\w+)\s*='
            concept_match = re.search(concept_pattern, remaining[:500], re.DOTALL)
            if concept_match:
                name = concept_match.group(1)
                # Find position in remaining
                concept_pos = remaining.find(f"concept {name}")
                if concept_pos >= 0:
                    # Extract the full concept definition (up to;)
                    concept_decl_end = remaining.find(';', concept_pos)
                    if concept_decl_end > 0:
                        # Extract template parameters from doxygen
                        template_param_list = []
                        if doxy and doxy.template_params:
                            template_param_list = list(doxy.template_params.keys())

                        # Create entry for concept
                        declarations.append(CppDecl(
                            kind='concept',
                            name=name,
                            template_params=template_param_list,
                            signature=f"concept {name}",
                            doxy=doxy,
                            member_types=[],
                            member_constants=[],
                            member_functions=[],
                            line=text[:pos_after_doxy].count('\n') + 1
                        ))
                continue

        # If not a struct/class, try inline constexpr variable
        if not decl_match:
            # Match: inline constexpr TYPE NAME{}; or TYPE NAME = value;
            var_pattern = r'\s*(?:inline\s+)?constexpr\s+(?:auto|\w+(?:<[^>]*>)?(?:\s*\*|&)?)\s+(\w+)\s*(?:\{[^}]*\}|=[^;]*)?;'
            var_match = re.match(var_pattern, remaining, re.DOTALL)
            if var_match:
                # Extract variable as a simple declaration
                name = var_match.group(1)
                var_text = var_match.group(0).strip()

                # Create a pseudo-struct entry for variables
                declarations.append(CppDecl(
                    kind='variable',
                    name=name,
                    template_params=[],
                    signature=var_text,
                    doxy=doxy,
                    member_types=[],
                    member_constants=[],
                    member_functions=[],
                    line=text[:pos_after_doxy].count('\n') + 1
                ))
                continue

        # If not a struct/class, try free function (template or not)
        if not decl_match:
            rem = remaining.lstrip()
            func_template_part = ""
            if rem.startswith('template'):
                angle_depth = 0
                start = rem.find('<')
                end = None
                for idx, ch in enumerate(rem[start:]):
                    if ch == '<':
                        angle_depth += 1
                    elif ch == '>':
                        angle_depth -= 1
                        if angle_depth == 0:
                            end = start + idx + 1
                            break
                if end:
                    func_template_part = rem[:end]
                    rem = rem[end:].lstrip()
            brace_idx = rem.find('{')
            if brace_idx != -1:
                func_sig = (func_template_part + " " + rem[:brace_idx]).strip()
                func_sig = re.sub(r'\s+', ' ', func_sig)
                name_match = list(re.finditer(r'(\w+)\s*\(', func_sig))
                func_name = name_match[-1].group(1) if name_match else None
                template_param_list = []
                if func_template_part:
                    tmatch = re.search(r'template\s*<([^>]*)>', func_template_part)
                    if tmatch:
                        tparams = tmatch.group(1)
                        for tparam in re.finditer(r'(?:typename|class)\s+(\w+)', tparams):
                            template_param_list.append(tparam.group(1))
                if func_name:
                    declarations.append(CppDecl(
                        kind='function',
                        name=func_name,
                        template_params=template_param_list,
                        signature=func_sig,
                        doxy=doxy,
                        member_types=[],
                        member_constants=[],
                        member_functions=[],
                        line=text[:pos_after_doxy].count('\n') + 1
                    ))
                    continue

        if not decl_match:
            continue

        template_part = decl_match.group(1) or ""
        requires_clause = decl_match.group(2) or ""  # New group for requires clause
        kind = decl_match.group(3)
        name = decl_match.group(4)
        inheritance = decl_match.group(5).strip()

        # Extract template parameter names from template part
        template_param_list = []
        if template_part:
            # Extract just the parameters from template<...>
            template_match = re.search(r'template\s*<([^>]*)>', template_part)
            if template_match:
                template_params_str = template_match.group(1)
                for tparam_match in re.finditer(r'(?:typename|class)\s+(\w+)', template_params_str):
                    template_param_list.append(tparam_match.group(1))

        # Build signature with inheritance
        sig = f"{kind} {name}"
        if inheritance and not inheritance.startswith(';'):
            sig += f" {inheritance}"

        # Normalize spaces in angle brackets for cleaner display
        sig = re.sub(r'>\s+<', '><', sig)  # Remove spaces between >< in nested templates
        sig = re.sub(r'<\s+', '<', sig)    # Remove space after <
        sig = re.sub(r'\s+>', '>', sig)    # Remove space before >

        # Find the matching closing brace
        brace_pos = decl_match.end() - 1
        brace_count = 1
        end_pos = brace_pos + 1
        while brace_count > 0 and end_pos < len(remaining):
            if remaining[end_pos] == '{':
                brace_count += 1
            elif remaining[end_pos] == '}':
                brace_count -= 1
            end_pos += 1

        struct_body = remaining[brace_pos:end_pos]

        # Extract members
        member_types, member_constants, member_functions = extract_struct_members(struct_body, doxy)

        line = text[:pos_after_doxy].count('\n') + 1

        declarations.append(CppDecl(
            kind=kind,
            name=name,
            template_params=template_param_list,
            signature=sig,
            doxy=doxy,
            member_types=member_types,
            member_constants=member_constants,
            member_functions=member_functions,
            line=line
        ))

    return declarations

def generate_wiki(include_dir: Path, output_dir: Path):
    """Generate comprehensive wiki pages matching cppreference.com style."""
    output_dir.mkdir(parents=True, exist_ok=True)

    files_with_docs = {}

    print(f"Parsing headers from {include_dir}...")
    for header in sorted(include_dir.rglob('*')):
        if header.is_dir() or header.name.startswith('.'):
            continue

        decls = extract_declarations(header)
        if decls:
            rel_path = str(header.relative_to(include_dir.parent)).replace('\\', '/')
            files_with_docs[rel_path] = decls
            total_items = sum(
                len(d.member_types) + len(d.member_constants) + len(d.member_functions) + 1
                for d in decls
            )
            print(f"  {rel_path}: {len(decls)} types, {total_items} total items")

    if not files_with_docs:
        print("No documented items found!")
        return

    # Generate per-file pages
    print(f"\nGenerating {len(files_with_docs)} wiki pages...")
    for rel_path, decls in files_with_docs.items():
        page_name = f"API-muesli-{rel_path.replace('/', '-').replace('muesli-', '')}.md"
        content = render_header_page(rel_path, decls)
        (output_dir / page_name).write_text(content, encoding='utf-8')

    # Generate convenience header pages (headers that just include others)
    print("Generating convenience header pages...")
    convenience_headers = {
        'include/muesli/codecs': {
            'title': 'All Codecs',
            'description': 'Convenience header that includes all codec implementations.',
            'includes': [
                ('fundamental_codecs', 'Fundamental type codecs'),
                ('stddef_codecs', 'Standard definition codecs'),
                ('stdint_codecs', 'Standard integer codecs'),
                ('string_codecs', 'String and text codecs'),
                ('array_codec', 'Fixed-size array codec'),
                ('range_codec', 'Range codec'),
                ('vector_codec', 'Vector codec'),
                ('nullable_codec', 'Nullable value codec'),
                ('optional_codec', 'Optional value codec'),
                ('tuple_codec', 'Tuple codec'),
                ('variant_codec', 'Variant codec'),
                ('constrained_codec', 'Constrained codec'),
                ('delimited_codec', 'Delimited codec'),
                ('transform_codec', 'Transform codec'),
            ]
        }
    }

    for header_path, info in convenience_headers.items():
        page_name = f"API-muesli-{header_path.replace('include/muesli/', '').replace('/', '-')}.md"
        content = render_convenience_header_page(info)
        (output_dir / page_name).write_text(content, encoding='utf-8')

    # Generate index
    index = render_index(files_with_docs)
    (output_dir / 'API-Reference.md').write_text(index, encoding='utf-8')

    print(f"Generated wiki in {output_dir} - {len(files_with_docs)} pages")

def render_declaration(decl: CppDecl) -> List[str]:
    """Render a single struct/class/variable declaration with comprehensive information."""
    lines = []

    # Header with name
    lines.append(f"## {decl.name}\n\n")

    # Signature box with template declaration
    lines.append("```cpp\n")
    if decl.template_params:
        # Check if signature already contains template declaration
        if not decl.signature.lstrip().startswith('template<'):
            lines.append(f"template<{', '.join(decl.template_params)}>\n")
    lines.append(f"{decl.signature}\n")
    lines.append("```\n\n")

    # Category badge if present
    if decl.doxy and decl.doxy.category:
        lines.append(f"**Category:** {decl.doxy.category}\n\n")

    # Brief description (always shown)
    if decl.doxy and decl.doxy.brief:
        brief_with_links = add_cppreference_links(decl.doxy.brief)
        lines.append(f"{brief_with_links}\n\n")

    # Detailed description
    if decl.doxy and decl.doxy.detailed:
        detailed_with_links = add_cppreference_links(decl.doxy.detailed)
        lines.append(f"{detailed_with_links}\n\n")

    # For variables and concepts, render examples then return
    if decl.kind in ('variable', 'concept'):
        # Example section for concepts
        if decl.doxy and decl.doxy.example:
            lines.append("### Example\n\n")
            lines.append("```cpp\n")
            lines.append(f"{decl.doxy.example}\n")
            lines.append("```\n\n")

        # See also for concepts
        if decl.doxy and decl.doxy.see_also:
            lines.append("### Related\n\n")
            for item in decl.doxy.see_also:
                linked_item = convert_see_also_to_link(item)
                lines.append(f"- {linked_item}\n")
            lines.append("\n")

        return lines

    # Template parameters table (only for struct/class, not functions)
    if decl.kind not in ('function',) and decl.doxy and decl.doxy.template_params:
        lines.append("### Template Parameters\n\n")
        lines.append("| Parameter | Description |\n")
        lines.append("|-----------|-------------|\n")
        for param_name, param_desc in decl.doxy.template_params.items():
            # Escape pipes in descriptions
            param_desc = param_desc.replace('|', '\\|')
            lines.append(f"| `{param_name}` | {param_desc} |\n")
        lines.append("\n")

    # Member types section
    if decl.member_types:
        lines.append("### Member Types\n\n")
        for mt in decl.member_types:
            lines.append(f"**`{mt.name}`**\n\n")
            lines.append("```cpp\n")
            # Clean signature to avoid duplication (signature may already include keyword)
            sig = mt.signature.strip()
            if not sig.startswith((mt.kind, f"{mt.kind} ")):
                sig = f"{mt.kind} {sig}"
            lines.append(f"{sig}\n")
            lines.append("```\n\n")
            if mt.doxy and mt.doxy.brief:
                brief_with_links = add_cppreference_links(mt.doxy.brief)
                lines.append(f"{brief_with_links}\n\n")

    # Member constants section
    if decl.member_constants:
        lines.append("### Member Constants\n\n")
        for mc in decl.member_constants:
            lines.append(f"**`{mc.name}`** -- `{mc.type_str}`\n\n")
            if mc.value:
                lines.append(f"```cpp\nstatic constexpr {mc.type_str} {mc.name} = {mc.value};\n```\n\n")
            if mc.doxy and mc.doxy.brief:
                brief_with_links = add_cppreference_links(mc.doxy.brief)
                lines.append(f"{brief_with_links}\n\n")

    # Member functions section
    if decl.member_functions:
        lines.append("### Member Functions\n\n")

        # Group by name for overloads
        by_name = defaultdict(list)
        for mf in decl.member_functions:
            by_name[mf.name].append(mf)

        for func_name in sorted(by_name.keys()):
            funcs = by_name[func_name]

            lines.append(f"#### `{func_name}`\n\n")

            # Brief description from first documented overload
            brief = ""
            detailed = ""
            for func in funcs:
                if func.doxy and func.doxy.brief:
                    brief = func.doxy.brief
                    if func.doxy.detailed:
                        detailed = func.doxy.detailed
                    break

            if brief:
                brief_with_links = add_cppreference_links(brief)
                lines.append(f"{brief_with_links}\n\n")

            if detailed:
                detailed_with_links = add_cppreference_links(detailed)
                lines.append(f"{detailed_with_links}\n\n")

            # All signatures
            lines.append("```cpp\n")
            for func in funcs:
                lines.append(f"{func.signature}\n")
            lines.append("```\n\n")

            # Parameters section (from first overload with params)
            params_added = False
            for func in funcs:
                if func.doxy and func.doxy.member_params:
                    if not params_added:
                        lines.append("**Parameters:**\n\n")
                        params_added = True
                    for param_name, param_desc in func.doxy.member_params.items():
                        lines.append(f"- `{param_name}` -- {param_desc}\n")

            if params_added:
                lines.append("\n")

            # Return description (from first overload with return)
            for func in funcs:
                if func.doxy and func.doxy.returns:
                    lines.append(f"**Returns:** {func.doxy.returns}\n\n")
                    break

            # Example (from first overload with example)
            for func in funcs:
                if func.doxy and func.doxy.example:
                    lines.append("**Example:**\n\n")
                    lines.append("```cpp\n")
                    lines.append(f"{func.doxy.example}\n")
                    lines.append("```\n\n")
                    break

    # For functions
    if decl.kind == 'function':
        # Template parameters table
        if decl.doxy and decl.doxy.template_params:
            lines.append("### Template Parameters\n\n")
            lines.append("| Parameter | Description |\n")
            lines.append("|-----------|-------------|\n")
            for param_name, param_desc in decl.doxy.template_params.items():
                param_desc = param_desc.replace('|', '\\|')
                lines.append(f"| `{param_name}` | {param_desc} |\n")
            lines.append("\n")

        # Parameters
        if decl.doxy and decl.doxy.member_params:
            lines.append("### Parameters\n\n")
            for param_name, param_desc in decl.doxy.member_params.items():
                lines.append(f"- `{param_name}` -- {param_desc}\n")
            lines.append("\n")

        # Returns
        if decl.doxy and decl.doxy.returns:
            lines.append(f"**Returns:** {decl.doxy.returns}\n\n")

        # Example
        if decl.doxy and decl.doxy.example:
            lines.append("### Example\n\n")
            lines.append("```cpp\n")
            lines.append(f"{decl.doxy.example}\n")
            lines.append("```\n\n")

        # Related
        if decl.doxy and decl.doxy.see_also:
            lines.append("### Related\n\n")
            for item in decl.doxy.see_also:
                lines.append(f"- {convert_see_also_to_link(item)}\n")
            lines.append("\n")

        return lines

    # Notes section
    if decl.doxy and decl.doxy.notes:
        lines.append("### Notes\n\n")
        lines.append(f"{decl.doxy.notes}\n\n")

    # Example section (if not in member functions and not empty)
    if decl.doxy and decl.doxy.example and not any(mf.doxy and mf.doxy.example for mf in decl.member_functions):
        lines.append("### Example\n\n")
        lines.append("```cpp\n")
        lines.append(f"{decl.doxy.example}\n")
        lines.append("```\n\n")

    # See also section
    see_also_items = []
    if decl.doxy and decl.doxy.see_also:
        see_also_items.extend(decl.doxy.see_also)

    # Extract base classes from signature
    # Need to be careful with angle brackets in templates
    base_match = re.search(r'\s:\s(.+)$', decl.signature)
    if base_match:
        bases_text = base_match.group(1)
        # Split on commas, but need to respect angle brackets
        bases = []
        current = ""
        depth = 0
        for char in bases_text:
            if char == '<':
                depth += 1
            elif char == '>':
                depth -= 1
            elif char == ',' and depth == 0:
                bases.append(current.strip())
                current = ""
                continue
            current += char
        if current.strip():
            bases.append(current.strip())

        # Add base classes to see also (but only first 2 to avoid clutter)
        for base in bases[:2]:
            # Clean up access specifier
            base_clean = re.sub(r'^\s*(public|protected|private)\s+', '', base)
            if base_clean and base_clean not in see_also_items:
                see_also_items.append(f"Inherits from: `{base_clean}`")

    # Add cross-links to related codec pages
    # Look for related codecs mentioned in brief/detailed descriptions
    full_text = (decl.doxy.brief if decl.doxy and decl.doxy.brief else "") + " " + \
                (decl.doxy.detailed if decl.doxy and decl.doxy.detailed else "")

    # Common codec names to link to
    codec_names = [
        'optional_codec', 'nullable_codec', 'variant_codec', 'array_codec',
        'vector_codec', 'range_codec', 'tuple_codec', 'delimited_codec',
        'constrained_codec', 'transform_codec', 'projector_codec', 'identity_codec',
        'member_codec', 'monostate_codec', 'apply_codec'
    ]

    related_codecs = []
    for codec_name in codec_names:
        if codec_name != decl.name and codec_name in full_text.lower():
            page_name = f"API-muesli-{codec_name}"
            if f"`{codec_name}`" not in '\n'.join(see_also_items):
                related_codecs.append(f"[`{codec_name}`]({page_name}#{codec_name})")

    if related_codecs:
        see_also_items.append("Related codecs: " + ", ".join(related_codecs))

    if see_also_items:
        lines.append("### Related\n\n")
        for item in see_also_items:
            # Convert @see items to links, but skip items that are already markdown links
            if item.startswith('[') or '](http' in item or '](' in item:
                # Already a link, don't convert
                linked_item = item
            else:
                # Plain text, convert to link if possible
                linked_item = convert_see_also_to_link(item)
            lines.append(f"- {linked_item}\n")
        lines.append("\n")

    return lines

def render_convenience_header_page(info: Dict) -> str:
    """Render a convenience header page that groups related includes."""
    lines = [
        f"# {info['title']}\n\n",
        f"{info['description']}\n\n",
        "## Included Headers\n\n",
    ]

    for header_name, header_desc in info['includes']:
        page_name = f"API-muesli-{header_name}"
        lines.append(f"- [`{header_name}`]({page_name}) -- {header_desc}\n")

    lines.append("\n")
    return ''.join(lines)

def render_header_page(rel_path: str, decls: List[CppDecl]) -> str:
    """Render a cppreference.com-style header page."""
    lines = [
        f"# {rel_path}\n\n",
        f"Defined in header `<{rel_path}>`\n\n",
    ]

    # Deduplicate declarations by name, preferring non-forward-declarations
    seen_names = {}
    for decl in decls:
        if decl.name not in seen_names:
            seen_names[decl.name] = decl
        else:
            # If current is not forward-decl and existing is, prefer current
            current_is_fwd = decl.doxy and 'forward declaration' in decl.doxy.brief.lower()
            existing_is_fwd = seen_names[decl.name].doxy and 'forward declaration' in seen_names[decl.name].doxy.brief.lower()
            if not current_is_fwd and existing_is_fwd:
                seen_names[decl.name] = decl

    for decl in seen_names.values():
        lines.extend(render_declaration(decl))
        lines.append("\n---\n\n")

    return ''.join(lines)

def render_index(files: Dict[str, List[CppDecl]]) -> str:
    """Render API reference index with organized sections."""
    lines = [
        "# API Reference\n\n",
        "Complete API documentation for muesli.\n\n",
    ]

    def _first_sentence(text: str) -> str:
        if not text:
            return ""
        m = re.match(r'^([^.!?\n]*[.!?]?)', text.strip())
        return m.group(1).strip() if m else text.strip()

    def _add_entry(lines_list, decl, page_name, seen):
        if decl.name in seen:
            return
        seen.add(decl.name)
        brief = _first_sentence(decl.doxy.brief if decl.doxy else "")
        brief = add_cppreference_links(brief) if brief else ""
        desc = f" -- {brief}" if brief else ""
        lines_list.append(f"- [`{decl.name}`]({page_name}#{decl.name}){desc}\n")

    seen_types = set()

    # Define sections with their organization
    sections = {
        'Codec Implementations': {
            'desc': 'Core codec types for serializing different data types',
            'subsections': {
                'Fundamental Types': ['fundamental_codecs', 'stddef_codecs', 'stdint_codecs'],
                'Strings': ['string_codecs'],
                'Containers': ['array_codec', 'vector_codec', 'range_codec'],
                'Composite Types': ['tuple_codec', 'variant_codec'],
                'Value Wrappers': ['optional_codec', 'nullable_codec'],
                'Transformations': ['constant_codec', 'identity_codec', 'constrained_codec', 'delimited_codec'],
                'Utility Codecs': ['monostate_codec', 'varint_codecs'],
            },
            'individual_types': ['member_codec', 'projector_codec'],
        },
        'Fluent API Mixins': {
            'desc': 'Trait-based mixins that extend codecs with method chains',
            'subsections': {
                'Core Mixins': ['fluent/constrainable', 'fluent/constructable', 'fluent/nullable',
                               'fluent/optionable', 'fluent/projectable', 'fluent/transformable'],
            },
        },
        'Format Handlers': {
            'desc': 'Serialization format implementations',
            'subsections': {
                'Binary Format': ['format/binary_format'],
            },
        },
        'Utilities': {
            'desc': 'Type traits, concepts, and helper utilities',
            'subsections': {
                'Type Traits': ['util/type_traits'],
                'Concepts': ['util/concepts'],
                'Ranges': ['util/range_holder'],
            },
        },
    }

    for section_name, section_info in sections.items():
        lines.append(f"## {section_name}\n\n")
        if 'desc' in section_info:
            lines.append(f"{section_info['desc']}\n\n")

        # Process subsections
        if 'subsections' in section_info:
            for subsection_name, header_names in section_info['subsections'].items():
                lines.append(f"### {subsection_name}\n\n")

                for header_name in sorted(header_names):
                    # Find matching file
                    for rel_path in sorted(files.keys()):
                        if rel_path.endswith(header_name):
                            page_name = f"API-muesli-{rel_path.replace('/', '-').replace('muesli-', '')}"
                            decls = files[rel_path]

                            # List only non-forward-declaration types
                            for decl in decls:
                                if decl.doxy and 'forward declaration' in decl.doxy.brief.lower():
                                    continue
                                _add_entry(lines, decl, page_name, seen_types)
                            break

                lines.append("\n")

        # Process individual types
        if 'individual_types' in section_info:
            for type_name in sorted(section_info['individual_types']):
                for rel_path in sorted(files.keys()):
                    if rel_path.endswith(type_name):
                        page_name = f"API-muesli-{rel_path.replace('/', '-').replace('muesli-', '')}"
                        decls = files[rel_path]
                        for decl in decls:
                            if decl.doxy and 'forward declaration' in decl.doxy.brief.lower():
                                continue
                            _add_entry(lines, decl, page_name, seen_types)
                        break

        lines.append("\n")

    return ''.join(lines)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Generate cppreference-style wiki from C++ headers')
    parser.add_argument('--output', default='temp_wiki', help='Output directory for wiki pages')
    args = parser.parse_args()

    generate_wiki(Path('include/muesli'), Path(args.output))
