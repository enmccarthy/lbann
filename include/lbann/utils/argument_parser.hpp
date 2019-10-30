////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2019, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
// Written by the LBANN Research Team (B. Van Essen, et al.) listed in
// the CONTRIBUTORS file. <lbann-dev@llnl.gov>
//
// LLNL-CODE-697807.
// All rights reserved.
//
// This file is part of LBANN: Livermore Big Artificial Neural Network
// Toolkit. For details, see http://software.llnl.gov/LBANN or
// https://github.com/LLNL/LBANN.
//
// Licensed under the Apache License, Version 2.0 (the "Licensee"); you
// may not use this file except in compliance with the License.  You may
// obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the license.
////////////////////////////////////////////////////////////////////////////////

#ifndef LBANN_UTILS_ARGUMENT_PARSER_HPP_INCLUDED
#define LBANN_UTILS_ARGUMENT_PARSER_HPP_INCLUDED

#include "lbann/utils/any.hpp"
#include "lbann/utils/environment_variable.hpp"

#include <clara.hpp>

#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace lbann
{
namespace utils
{

/** @class argument_parser
 *  @brief A decorator class over Clara
 */
class argument_parser
{
public:

  /** @name Public types */
  ///@{

  /** @brief A proxy class representing the current value associated
   *         with an option.
   *
   *  This class is best manipulated generically, through `auto`
   *  variables.
   *
   *  @tparam T The type of the held object.
   */
  template <typename T>
  class readonly_reference
  {
  public:
    readonly_reference(T& val) noexcept : ref_(val) {}
    T const& get() const noexcept { return ref_; }
    operator T const& () const noexcept { return this->get(); }

    template <typename S>
    bool operator==(S const& y) const noexcept
    { return this->get() == y; }

  private:
    T& ref_;
  };// class readonly_reference<T>

  /** @class parse_error
   *  @brief std::exception subclass that is thrown if the parser
   *         can not parse the arguments.
   */
  struct parse_error : std::runtime_error
  {
    /** @brief Construct the exception with the string to be
     *         return by what()
     */
    template <typename T>
    parse_error(T&& what_arg)
      : std::runtime_error{std::forward<T>(what_arg)} {}
  };

  /** @class missing_required_arguments
   *  @brief std::exception subclass that is thrown if a required
   *         argument is not found.
   */
  struct missing_required_arguments : std::runtime_error
  {
    /** @brief Construct the exception with a list of the missing
     *         argument names.
     *
     *  @param[in] missing_args A container that holds the names
     *             of the missing arguments.
     */
    template <typename Container>
    missing_required_arguments(Container const& missing_args)
      : std::runtime_error{build_what_string_(missing_args)}
    {}

  private:
    template <typename Container>
    std::string build_what_string_(Container const& missing_args)
    {
      std::ostringstream oss;
      oss << "The following required arguments are missing: {";
      for (auto const& x : missing_args)
        oss << " \"" << x << "\"";
      oss << " }";
      return oss.str();
    }
  };

  ///@}

public:

  /** @name Constructors */
  ///@{

  /** @brief Create the parser */
  argument_parser();

  ///@}
  /** @name Adding options and arguments */
  ///@{

  /** @brief Add a flag (i.e. a boolean parameter that is "true" if
   *         given and "false" if not given.
   *
   *  The value of a flag defaults to `false`. If, for some strange
   *  reason, users should be forced to type the boolean value on
   *  the command line, e.g., "my_exe -b 1", use add_option()
   *  instead. If a flag with default value `true` is desired,
   *  invert the logic and use this instead.
   *
   *  @param[in] name The name to be used to refer to the argument.
   *  @param[in] cli_flags The valid command line flags to trigger
   *             this flag to `true`. At least one must be given.
   *  @param[in] description A brief description of the argument,
   *             used for the help message.
   */
  readonly_reference<bool>
  add_flag(std::string const& name,
           std::initializer_list<std::string> cli_flags,
           std::string const& description);

  template <typename AccessPolicy>
  readonly_reference<bool>
  add_flag(std::string const& name,
           std::initializer_list<std::string> cli_flags,
           EnvVariable<AccessPolicy> env,
           std::string const& description)
  {
    if (env.exists() && env.template value<bool>())
      return add_flag_impl_(name, std::move(cli_flags), description, true);
    else
      return add_flag(name, std::move(cli_flags), description);
  }

  /** @brief Add an additional named option.
   *
   *  Currently, named options are all optional. This could be
   *  expanded if needed.
   *
   *  @tparam T The type associated with the option. Deduced if a
   *          default value is given. If the default value is not
   *          given, the template parameter must be named explicitly
   *          and the default value will be default-constructed.
   *
   *  @param[in] name The name to be used to refer to the argument.
   *  @param[in] cli_flags The valid command line flags to identify
   *             this option and its value. At least one must be
   *             given.
   *  @param[in] description A brief description of the argument,
   *             used for the help message.
   *  @param[in] default_value The default value to be returned if
   *             the option is not passed to the command line.
   */
  template <typename T>
  readonly_reference<T>
  add_option(std::string const& name,
             std::initializer_list<std::string> cli_flags,
             std::string const& description,
             T default_value = T());

  /** @brief Add an additional named option.
   *
   *  Currently, named options are all optional. This could be
   *  expanded if needed.
   *
   *  @tparam T The type associated with the option. Deduced if a
   *          default value is given. If the default value is not
   *          given, the template parameter must be named explicitly
   *          and the default value will be default-constructed.
   *  @tparam AccessPolicy The access method for the environment
   *          variable. (Deduced.)
   *
   *  @param[in] name The name to be used to refer to the argument.
   *  @param[in] cli_flags The valid command line flags to identify
   *             this option and its value. At least one must be
   *             given.
   *  @param[in] env The environment variable to prefer over the
   *             default parameter value.
   *  @param[in] description A brief description of the argument,
   *             used for the help message.
   *  @param[in] default_value The default value to be returned if
   *             the option is not passed to the command line.
   */
  template <typename T, typename AccessPolicy>
  readonly_reference<T>
  add_option(std::string const& name,
             std::initializer_list<std::string> cli_flags,
             EnvVariable<AccessPolicy> env,
             std::string const& description,
             T default_value = T())
  {
    if (env.exists())
      return add_option(name, std::move(cli_flags), description,
                        env.template value<T>());
    else
      return add_option(name, std::move(cli_flags), description,
                        std::move(default_value));
  }

  /** @brief Add an additional named option; overloaded for "char
   *         const*" parameters.
   *
   *  The value will be stored as an `std::string`. Its value must
   *  be extracted using `get<std::string>(name)`.
   *
   *  @param[in] name The name to be used to refer to the argument.
   *  @param[in] cli_flags The valid command line flags to trigger
   *             this flag to `true`. At least one must be given.
   *  @param[in] description A brief description of the argument,
   *             used for the help message.
   *  @param[in] default_value The default value to be returned if
   *             the option is not passed to the command line.
   */
  readonly_reference<std::string>
  add_option(std::string const& name,
             std::initializer_list<std::string> cli_flags,
             std::string const& description,
             char const* default_value)
  {
    return add_option(name, std::move(cli_flags), description,
                      std::string(default_value));
  }

  /** @brief Add an additional named option; overloaded for "char
   *         const*" parameters.
   *
   *  The value will be stored as an `std::string`. Its value must
   *  be extracted using `get<std::string>(name)`.
   *
   *  @param[in] name The name to be used to refer to the argument.
   *  @param[in] cli_flags The valid command line flags to trigger
   *             this flag to `true`. At least one must be given.
   *  @param[in] env The environment variable to prefer over the
   *             default parameter value.
   *  @param[in] description A brief description of the argument,
   *             used for the help message.
   *  @param[in] default_value The default value to be returned if
   *             the option is not passed to the command line.
   */
  template <typename AccessPolicy>
  readonly_reference<std::string>
  add_option(std::string const& name,
                  std::initializer_list<std::string> cli_flags,
                  EnvVariable<AccessPolicy> env,
                  std::string const& description,
                  char const* default_value)
  {
    return add_option(name, cli_flags, std::move(env),
                      description, std::string(default_value));
  }

  /** @brief Add an optional positional argument.
   *
   *  @tparam T The type to which the argument maps.
   *
   *  @param[in] name The name to be used to refer to the argument.
   *  @param[in] description A brief description of the argument,
   *             used for the help message.
   *  @param[in] default_value The value to use for this argument if
   *             not detected in the formal argument list.
   *
   *  @return The index of the positional argument at the time it is
   *          added. Required arguments get ordered before optional
   *          arguments, so this number might change if required
   *          arguments are added after this argument.
   */
  template <typename T>
  size_t add_argument(
    std::string const& name,
    std::string const& description,
    T default_value = T());

  /** @brief Add a positional argument; char const* overload
   *
   *  The data is stored in an std::string object internally and
   *  must be accessed using `get<std::string>(name)`.
   *
   *  @param[in] name The name to be used to refer to the argument.
   *  @param[in] description A brief description of the argument,
   *             used for the help message.
   *  @param[in] default_value The value to use for this argument if
   *             not detected in the formal argument list.
   *
   *  @return The index of the positional argument
   */
  size_t add_argument(
    std::string const& name,
    std::string const& description,
    char const* default_value)
  {
    return add_argument(
      name, description, std::string(default_value));
  }

  /** @brief Add a "required" positional argument.
   *
   *  @tparam T The type to which the argument maps.
   *
   *  @param[in] name The name to be used to refer to the argument.
   *  @param[in] description A brief description of the argument,
   *             used for the help message.
   *
   *  @return The index of the positional argument. Required
   *          arguments will be ordered before non-required
   *          arguments, so this will never change.
   */
  template <typename T>
  size_t add_required_argument(
    std::string const& name,
    std::string const& description);

  ///@}
  /** @name Command-line-like parsing */
  ///@{

  /** @brief Parse the command line arguments and finalize the
   *         arguments.
   *
   *  This is equivalent to calling parse_no_finalize() followed
   *  immediately by finalize().
   *
   *  @param[in] argc The number of arguments
   *  @param[in] argv The list of arguments
   *
   *  @throws parse_error if an internal parsing error is detected.
   */
  void parse(int argc, char const* const argv[]);

  /** @brief Parse the command line arguments but do not finalize
   *         the parser.
   *
   *  This parses command-line-like arguments but does no checks for
   *  required arguments. Users should call finalize() before
   *  attempting to use the values associated with any required
   *  arguments.
   *
   *  @param[in] argc The number of arguments
   *  @param[in] argv The list of arguments
   *
   *  @throws parse_error if an internal parsing error is detected.
   */
  void parse_no_finalize(int argc, char const* const argv[]);

  /** @brief Assert that all required components are set properly.
   *

   *  This should be called sometime after parse_no_finalize() and
   *  before using the values. This is implicitly called by parse().
   *
   *  @throws missing_required_arguments If a missing argument is
   *          detected.
   */
  void finalize() const;

  ///@}
  /** @name Queries */
  ///@{

  /** @brief Get the executable name.
   *
   *  This is only meaningful after calling either parse() or
   *  parse_no_finalize().
   *
   *  @return The name of the executable.
   */
  std::string const& get_exe_name() const noexcept;

  /** @brief Test if an option exists in the parser.
   *
   *  This only tests whether the argument or option is known to the
   *  parser, not whether it has been set or modified by the parser.
   *
   *  @param[in] option_name The name of the option/argument.
   */
  bool option_is_defined(std::string const& option_name) const;

  /** @brief Test if help has been requested. */
  bool help_requested() const;

  /** @brief Get the requested value from the argument list.
   *  @tparam T The type of the requested parameter.
   *  @param option_name The name given to the option or argument.
   *  @return A const-reference to the held value.
   */
  template <typename T>
  T const& get(std::string const& option_name) const;

  ///@}
  /** @name Output */
  ///@{

  /** @brief Print a help string to a stream.
   *  @param[in] stream The ostream to print the help message to.
   */
  void print_help(std::ostream& stream) const;

  ///@}

private:

  /** @brief Implementation of add_flag */
  readonly_reference<bool>
  add_flag_impl_(std::string const& name,
                 std::initializer_list<std::string> cli_flags,
                 std::string const& description,
                 bool default_value);

private:
  /** @brief Dictionary of arguments to their values */
  std::unordered_map<std::string, utils::any> params_;
  /** @brief Patch around in-progress clara limitation */
  std::unordered_set<std::string> required_;
  /** @brief The underlying clara object */
  clara::Parser parser_;
  /** @brief The name of the executable. */
  std::string exe_name_ = "<exe>";

};

inline bool
argument_parser::option_is_defined(std::string const& option_name) const
{
  return params_.count(option_name);
}

template <typename T>
inline T const& argument_parser::get(std::string const& option_name) const
{
  return utils::any_cast<T const&>(params_.at(option_name));
}

template <typename T>
inline auto argument_parser::add_option(
  std::string const& name,
  std::initializer_list<std::string> cli_flags,
  std::string const& description,
  T default_value)
  -> readonly_reference<T>
{
  params_[name] = std::move(default_value);
  auto& param_ref = any_cast<T&>(params_[name]);
  clara::Opt option(param_ref, name);
  for (auto const& f : cli_flags)
    option[f];
  parser_ |= option(description).optional();
  return param_ref;
}

template <typename T>
inline size_t argument_parser::add_argument(
  std::string const& name,
  std::string const& description,
  T default_value)
{
  params_[name] = std::move(default_value);
  parser_ |= clara::Arg
    (utils::any_cast<T&>(params_[name]), name)
    (description).optional();
  return parser_.m_args.size() - 1;
}

template <typename T>
inline size_t argument_parser::add_required_argument(
  std::string const& name,
  std::string const& description)
{
  // Add the reference to bind to
  params_[name] = T{};
  auto& param_ref = params_[name];

  required_.insert(name);

  // Make sure the required arguments are all grouped together.
  auto iter = parser_.m_args.cbegin(), invalid = parser_.m_args.cend();
  while (iter != invalid && !iter->isOptional())
    ++iter;

  // Create the argument
  auto ret = parser_.m_args.emplace(
    iter,
    [name,&param_ref,this](std::string const& value)
    {
      auto result = clara::detail::convertInto(
        value, utils::any_cast<T&>(param_ref));
      if (result)
        required_.erase(name);
      return result;
    },
    name);
  ret->operator() (description).required();
  return std::distance(parser_.m_args.begin(), ret);
}
}// namespace utils

utils::argument_parser& global_argument_parser();

}// namespace lbann

/** @brief Write the parser's help string to the given @c ostream */
std::ostream& operator<<(std::ostream&, lbann::utils::argument_parser const&);

#endif /* LBANN_UTILS_ARGUMENT_PARSER_HPP_INCLUDED */
