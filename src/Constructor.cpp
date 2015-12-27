#include "Constructor.h"
#include <unordered_map>
#include <cassert>

Module Constructor::construct(const std::vector<Token>& tokens) {
	this->tokens = &tokens;
	index = 0;
	return do_module();
}

Module Constructor::do_module() {
	Module module;
	while (true) {
		auto item = do_item();
		module.push_back(std::move(item));
		if (index >= tokens->size()) break;
	}
	return module;
}

std::unique_ptr<Item> Constructor::do_item() {
	const Token& token = next();
	switch (token.form) {
		case Token::KW_FN: return do_function();
		default: throw Exception("Expected 'fn'.", token);
	}
}

std::unique_ptr<Function> Constructor::do_function() {
	const Token& name_token = expect_ident();
	std::unique_ptr<Function> function(new Function(name_token));
	expect("(");

	// parameters
	const Token* param_token = &next();
	while (param_token->str != ")") {
		if (param_token->form != Token::IDENT) {
			throw Exception("Expected ident", *param_token);
		}
		function->param_names.push_back(param_token);
		expect(":");
		const Token& type_token = expect_ident();
		function->param_types.push_back(parse_type(type_token.str));
		param_token = &next();
		if (param_token->str == ",") param_token = &next();
		else if (param_token->str != ")") throw Exception("Expected ',' or ')'", *param_token);
	}

	// return type
	const Token* colon_token = &next();
	if (colon_token->str == ":") {
		const Token& ret_token = expect_ident();
		function->return_type = parse_type(ret_token.str);
		colon_token = &next();
	}

	if (colon_token->str != "{") throw Exception("Expected ':' or '{'", *colon_token);
	function->block = do_block();
	return std::move(function);
}

Block Constructor::do_block() {
	Block block;
	while (true) {
		if (peek() == Token(Token::SYMBOL, "}")) {
			next();
			break;
		}
		auto statement = do_statement();
		block.push_back(std::move(statement));
	}
	return block;
}

std::unique_ptr<Statement> Constructor::do_statement() {
	const Token& token = next();
	switch (token.form) {
		case Token::IDENT: {
			const Token& token2 = next();
			if      (token2.str == ":") return do_declare(token);
			else if (token2.str == "=") return do_assign(token, Op::NOT, nullptr);
			else {
				expect("=");
				switch (token2.str[0]) {
					case '+': return do_assign(token, Op::ADD,  &token2);
					case '-': return do_assign(token, Op::SUB,  &token2);
					case '*': return do_assign(token, Op::MUL,  &token2);
					case '/': return do_assign(token, Op::DIV,  &token2);
					case '&': return do_assign(token, Op::BAND, &token2);
					case '|': return do_assign(token, Op::BOR,  &token2);
					case '^': return do_assign(token, Op::XOR,  &token2);
					default: throw Exception("Expected ':', '=', or an operator", token2);
				}
			}
		}
		case Token::KW_RETURN: {
			std::unique_ptr<Return> return_statement(new Return(token));
			return_statement->value.reset(new Expr());
			do_expr(*return_statement->value);
			return std::move(return_statement);
		}
		case Token::KW_IF:    return do_if(token);
		case Token::KW_WHILE: return do_while(token);
		case Token::KW_BREAK: return do_break(token);
		case Token::KW_CONTINUE:
			expect(";");
			return std::unique_ptr<Statement>(new Statement(token, Statement::CONTINUE));
		default: throw Exception("Invalid statement", token);
	}
}

std::unique_ptr<Declaration> Constructor::do_declare(const Token& ident) {
	const Token& token = next();
	std::unique_ptr<Declaration> declaration;
	if (token == Token(Token::SYMBOL, "=")) { // var := val
		declaration.reset(new Declaration(ident));
	} else if (token.form == Token::IDENT) {  // var: type
		const Token& eq_token = next();
		if (eq_token == Token(Token::SYMBOL, ";")) {
			return std::unique_ptr<Declaration>(new Declaration(ident, token));
		}
		if (eq_token != Token(Token::SYMBOL, "=")) throw Exception("Expected '='", eq_token);
		declaration.reset(new Declaration(ident, token));
	} else {
		throw Exception("Expected '=' or identifier.", token);
	}
	Expr* expr = new Expr();
	do_expr(*expr);
	declaration->value.reset(expr);
	return declaration;
}

std::unique_ptr<Assignment> Constructor::do_assign(
		const Token& ident, Op op, const Token* op_token
) {
	Expr* expr = new Expr();
	if (op != Op::NOT) {
		// If it is an operator assignment (like +=) then the expression has the variable
		// appended to the front and the operator appended to the back.
		expr->push_back(Tok(ident, Tok::VAR, Type::UNKNOWN));
		do_expr(*expr);
		expr->push_back(Tok(*op_token, op));
	} else {
		do_expr(*expr);
	}
	std::unique_ptr<Assignment> assignment(new Assignment(ident));
	assignment->value.reset(expr);
	return assignment;
}

std::unique_ptr<If> Constructor::do_if(const Token& kw) {
	std::unique_ptr<If> if_statement(new If(kw));
	if_statement->conditions.emplace_back(new Expr());
	do_expr(*if_statement->conditions[0], "{");
	if_statement->blocks.push_back(do_block());
	while (peek().form == Token::KW_ELSE) {
		next();
		const Token& if_tok = next();
		if (if_tok.form == Token::KW_IF) {
			if_statement->conditions.emplace_back(new Expr());
			do_expr(*if_statement->conditions.back(), "{");
			if_statement->blocks.push_back(do_block());
		} else if (if_tok == Token(Token::SYMBOL, "{")){
			if_statement->blocks.push_back(do_block());
		} else {
			throw Exception("Expected 'if' or '{'", if_tok);
		}
	}
	return std::move(if_statement);
}

std::unique_ptr<While> Constructor::do_while(const Token& kw) {
	std::unique_ptr<While> while_statement(new While(kw));
	while_statement->condition.reset(new Expr());
	if (peek().str == "{") {
		while_statement->condition->push_back(Tok(kw, true));
	} else {
		do_expr(*while_statement->condition, "{");
	}
	while_statement->block = do_block();
	return while_statement;
}

std::unique_ptr<Break> Constructor::do_break(const Token& kw) {
	std::unique_ptr<Break> break_statement(new Break(kw));
	const Token& token = next();
	if (token.str != ";") {
		if (token.str != "*") throw Exception("Expected ';' or '*'", token);
		const Token& amount = next();
		if (amount.form != Token::INT) throw Exception("Expected integer", amount);
		break_statement->amount = (int)amount.i;
		expect(";");
	}
	return break_statement;
}

void Constructor::do_expr(Expr& expr, const std::string& terminator) {
	std::vector<Op> ops;
	std::vector<const Token*> tokens;
	bool prev_was_op = true;
	while (true) {
		const Token& token = peek();
		if (token == Token(Token::SYMBOL, terminator)) {
			// the expression has terminated
			while (!ops.empty()) {
				if (ops.back() == Op::TEMP_PAREN) throw Exception("Unclosed parenthesis.", token);
				expr.push_back(Tok(*tokens.back(), ops.back(), return_type(ops.back())));
				tokens.pop_back();
				ops.pop_back();
			}
			next();
			return;
		}
		bool pwo = prev_was_op;
		prev_was_op = false;
		next();
		switch (token.form) {
			case Token::INT:   expr.push_back(Tok(token, token.i, token.type)); break;
			case Token::FLOAT: expr.push_back(Tok(token, token.f, token.type)); break;
			case Token::KW_TRUE:  expr.push_back(Tok(token, true)); break;
			case Token::KW_FALSE: expr.push_back(Tok(token, false)); break;
			case Token::IDENT:
				if (peek().str == "(") {
					ops.push_back(Op::TEMP_FUNC);
					tokens.push_back(&token);
				} else {
					expr.push_back(Tok(token, Tok::VAR));
				}
				break;
			case Token::SYMBOL: {
				if (token.str[0] == ',') {
					while (true) {
						if (ops.empty()) throw Exception("Mismatched parenthesis.", token);
						Op op = ops.back();
						if (op == Op::TEMP_PAREN) {
							break;
						} else {
							expr.push_back(Tok(*tokens.back(), op, return_type(op)));
							ops.pop_back();
							tokens.pop_back();
						}
					}
					break;
				} else if (token.str[0] == '(') {
					ops.push_back(Op::TEMP_PAREN);
					tokens.push_back(nullptr);
					prev_was_op = true;
					break;
				} else if (token.str[0] == ')') {
					while (true) {
						if (ops.empty()) throw Exception("Mismatched parenthesis.", token);
						Op op = ops.back();
						ops.pop_back();
						if (op == Op::TEMP_PAREN) {
							tokens.pop_back();
							if (!ops.empty() && ops.back() == Op::TEMP_FUNC) {
								ops.pop_back();
								expr.push_back(Tok(*tokens.back(), Tok::FUNCTION));
								tokens.pop_back();
							}
							break;
						} else {
							expr.push_back(Tok(*tokens.back(), op, return_type(op)));
							tokens.pop_back();
						}
					}
					break;
				}

				static std::unordered_map<std::string, Op> operators = {
					{"+", Op::ADD}, {"-", Op::SUB}, {"*", Op::MUL}, {"/", Op::DIV}, {"%", Op::MOD},
					{"<<", Op::LSH},{">>", Op::RSH},{"&", Op::BAND},{"|", Op::BOR}, {"^", Op::XOR},
					{"==", Op::EQ}, {"!=", Op::NEQ},{"&&", Op::AND},{"||", Op::OR},
					{"!", Op::NOT}, {">", Op::GT}, {"<", Op::LT}, {">=", Op::GEQ}, {"<=", Op::LEQ}
				};
				auto iter = operators.find(token.str);
				if (iter == operators.end()) throw Exception("Unexpected symbol.", token);
				Op op = iter->second;
				if (op == Op::SUB && pwo) op = Op::NEG;
				if (op == Op::DIV && pwo) op = Op::INV;
				while (true) {
					if (ops.empty()) break;
					Op op2 = ops.back();
					if (!((left_assoc(op) && precedence(op) <= precedence(op2)) ||
					      (!left_assoc(op) && precedence(op) < precedence(op2)))) {
						break;
					}
					ops.pop_back();
					tokens.pop_back();
					expr.push_back(Tok(token, op2, return_type(op2)));
				}
				prev_was_op = true;
				ops.push_back(op);
				tokens.push_back(&token);
			} break;
			default: throw Exception("Unexpected token.", token);
		}
	}
}

const Token& Constructor::next() {
	if (index >= tokens->size()) throw Exception("Unterminated statement.", (*tokens)[index - 1]);
	return (*tokens)[index++];
}
const Token& Constructor::peek() {
	if (index >= tokens->size()) {
		static Token end(Token::INVALID, "");
		return end;
	}
	return (*tokens)[index];
}
void Constructor::expect(const std::string& str) {
	const Token& token = next();
	if (token.str != str) throw Exception("Expected '" + str + "'", token);
}
const Token& Constructor::expect_ident() {
	const Token& token = next();
	if (token.form != Token::IDENT) throw Exception("Expected ident", token);
	return token;
}


Type Constructor::return_type(Op op) {
	switch (op) {
		case Op::NEG: case Op::ADD: case Op::MUL: case Op::MOD:
		case Op::INV: case Op::SUB: case Op::DIV:
		case Op::BAND: case Op::BOR: case Op::XOR: case Op::LSH: case Op::RSH:
			return Type::NUM;

		case Op::NOT: case Op::AND: case Op::OR: case Op::EQ: case Op::NEQ:
		case Op::GT: case Op::LT: case Op::GEQ: case Op::LEQ:
			return Type::BOOL;

		case Op::TEMP_PAREN:
			assert(false);
			return Type::INVALID;
	}
}
bool Constructor::left_assoc(Op op) {
	switch (op) {
		case Op::NEG: case Op::INV: case Op::NOT: return false;
		default: return true;
	}
}
int Constructor::precedence(Op op) {
	switch (op) {
		case Op::NEG: case Op::NOT: case Op::INV:             return 10;
		case Op::MUL: case Op::DIV: case Op::MOD:             return  9;
		case Op::ADD: case Op::SUB:                           return  8;
		case Op::LSH: case Op::RSH:                           return  7;
		case Op::LEQ: case Op::GEQ: case Op::LT: case Op::GT: return  6;
		case Op::EQ:  case Op::NEQ:                           return  5;
		case Op::BAND:                                        return  4;
		case Op::XOR:                                         return  3;
		case Op::BOR:                                         return  2;
		case Op::AND:                                         return  1;
		case Op::OR:                                          return  0;
		case Op::TEMP_PAREN:                                  return -1;
	}
}


Constructor::Exception::Exception(std::string desc, Token token):
		std::invalid_argument(desc), desc(desc), token(token) {
	std::stringstream ss;
	ss << "line " << token.line << ", column " << token.column << " (" << token.str << "): " << desc;
	this->desc = ss.str();
}

const char* Constructor::Exception::what() const throw() {
	return desc.c_str();
}
