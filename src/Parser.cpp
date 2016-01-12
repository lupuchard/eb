#include "Parser.h"
#include "Exception.h"

void Parser::construct(Module& module, const std::vector<Token>& tokens) {
	this->tokens = &tokens;
	index = 0;
	do_module(module);
}

void Parser::do_module(Module& module) {
	while (true) {
		trim();
		if (index >= tokens->size()) break;
		auto item = do_item();
		module.push_back(std::move(item));
	}
}

std::unique_ptr<Item> Parser::do_item() {
	const Token* token = &next();
	bool pub = false;
	if (token->form == Token::KW_PUB) {
		pub = true;
		token = &next();
	}
	std::unique_ptr<Item> item;
	if (token->form == Token::KW_FN) {
		item = do_function();
	} else if (token->str() == "import") {
		item = do_import(*token);
	} else if (token->str() == "global") {
		item = do_global(false);
	} else if (token->str() == "const") {
		item = do_global(true);
	} else {
		throw Exception("Expected item", *token);
	}
	item->pub = pub;
	return item;
}

std::unique_ptr<Import> Parser::do_import(const Token& kw) {
	std::unique_ptr<Import> import(new Import(kw));
	while (true) {
		const Token& next_token = next();
		import->target.push_back(&next_token);
		if (next_token.str() == "[") {
			import->target.push_back(&next_token);
			while (true) {
				const Token& nexter_token = next();
				import->target.push_back(&nexter_token);
				if (nexter_token.form == Token::IDENT) {
					if (peek().str() == "]") {
						import->target.push_back(&next());
						break;
					}
				} else if (nexter_token.str() == "]") {
					break;
				}
				expect(",");
			}
		} else if (!(next_token.form == Token::IDENT || next_token.str() == "*")) {
			throw Exception("Invalid import syntax", kw);
		}
		if (peek().form == Token::END) {
			next();
			break;
		}
		expect(".");
	}
	return import;
}

std::unique_ptr<Function> Parser::do_function() {
	const Token& name_token = expect_ident();
	assert_simple_ident(name_token);
	std::unique_ptr<Function> function(new Function(name_token));
	expect("(");
	trim();

	// parameters
	const Token* param_token = &next();
	while (param_token->str() != ")") {
		assert_simple_ident(*param_token);
		function->param_names.push_back(param_token);
		expect(":");
		const Token& type_token = expect_ident();
		function->param_types.push_back(Type::parse(type_token.str()));
		assert(function->param_types.back() != Type::Invalid);
		param_token = &next();
		trim();
		if (param_token->str() == ",") param_token = &next();
		else if (param_token->str() != ")") throw Exception("Expected ',' or ')'", *param_token);
	}
	assert(function->param_types.size() < 256);

	// return type
	const Token* colon_token = &next();
	if (colon_token->str() == ":") {
		trim();
		const Token& ret_token = expect_ident();
		function->return_type = Type::parse(ret_token.str());
		colon_token = &next();
	}
	trim();

	if (colon_token->str() != "{") throw Exception("Expected ':' or '{'", *colon_token);
	function->block = do_block();
	return std::move(function);
}

std::unique_ptr<Global> Parser::do_global(bool conzt) {
	const Token& name = next();
	expect(":");
	const Token& type_token = expect_ident();
	return std::unique_ptr<Global>(new Global(name, Type::parse(type_token.str()), conzt));
}

Block Parser::do_block() {
	Block block;
	while (true) {
		trim();
		if (peek() == Token(Token::SYMBOL, "}")) {
			next();
			break;
		}
		auto statement = do_statement();
		block.push_back(std::move(statement));
	}
	return block;
}

struct Op {
	Op(int precidence, int num_params = 2, bool left_assoc = true)
			: precidence(precidence), num_params(num_params), left_assoc(left_assoc) { }
	int precidence;
	int num_params;
	bool left_assoc;
};
Op ADD(8); Op SUB(8);
Op MUL(9); Op DIV(9); Op MOD(9);
Op NEG(10, 1, false); Op INV(10, 1, false);
Op BAND(4); Op BOR(2); Op XOR(3);
Op LSH(7); Op RSH(7);
Op AND(1); Op OR(0); Op NOT(10, 1, false);
Op LEQ(6); Op GEQ(6); Op LT(6); Op GT(6);
Op EQ(5); Op NEQ(5);
Op FUNC(-1, -1);
Op PAREN(-1, -1);

std::unique_ptr<Statement> Parser::do_statement() {
	const Token& token = next();
	switch (token.form) {
		case Token::IDENT: {
			const Token& token2 = peek();
			if (token2.str() == ":") {
				next();
				return do_declare(token);
			} else if (token2.str() == "=") {
				next();
				return do_assign(token, nullptr);
			} else if (peek(2).str() == "=") {
				next(2);
				switch (token2.str()[0]) {
					case '+': case '-': case '*': case '/':
					case '&': case '|': case '^':
						return do_assign(token,  &token2);
					default: throw Exception("Expected ':', '=', or an operator", token2);
				}
			} else {
				return do_expr(token);
			}
		}
		case Token::KW_RETURN: return do_return(token);
		case Token::KW_IF: {
			index--;
			std::unique_ptr<Statement> statement(new Statement(token, Statement::EXPR));
			do_expr(statement->expr);
			if (statement->expr.size() == 1) {
				std::unique_ptr<If> if_statement;
				if_statement.swap(((IfTok&)*statement->expr[0]).if_statement);
				return std::move(if_statement);
			}
			return statement;
		}
		case Token::KW_WHILE:  return do_while(token);
		case Token::KW_BREAK:  return do_break(token);
		case Token::KW_CONTINUE:
			return std::unique_ptr<Statement>(new Statement(token, Statement::CONTINUE));
		default: return do_expr(token);
	}
}

std::unique_ptr<Declaration> Parser::do_declare(const Token& ident) {
	const Token& token = next();
	std::unique_ptr<Declaration> declaration;
	if (token.str() == "=") { // var := val
		declaration.reset(new Declaration(ident));
	} else if (token.form == Token::IDENT) {  // var: type
		assert_simple_ident(token);
		const Token& eq_token = next();
		if (eq_token.form == Token::END) {
			return std::unique_ptr<Declaration>(new Declaration(ident, token));
		}
		if (eq_token.str() != "=") throw Exception("Expected '='", eq_token);
		declaration.reset(new Declaration(ident, token));
	} else if (token.form == Token::END) {
		declaration.reset(new Declaration(ident));
		return declaration;
	} else {
		throw Exception("Expected '=' or identifier.", token);
	}
	do_expr(declaration->expr);
	return declaration;
}

std::unique_ptr<Statement> Parser::do_assign(const Token& ident, const Token* op_token) {
	std::unique_ptr<Statement> assignment(new Statement(ident, Statement::ASSIGNMENT));
	if (op_token != nullptr) {
		// If it is an operator assignment (like +=) then the expression has the variable
		// appended to the front and the operator appended to the back.
		assignment->expr.emplace_back(new VarTok(ident));
		do_expr(assignment->expr);
		assignment->expr.emplace_back(new FuncTok(*op_token, 2));
	} else {
		do_expr(assignment->expr);
	}
	return assignment;
}

std::unique_ptr<Statement> Parser::do_expr(const Token& kw) {
	std::unique_ptr<Statement> expression(new Statement(kw, Statement::EXPR));
	index--;
	do_expr(expression->expr);
	return expression;
}

std::unique_ptr<Statement> Parser::do_return(const Token& kw) {
	std::unique_ptr<Statement> return_statement(new Statement(kw, Statement::RETURN));
	do_expr(return_statement->expr);
	return return_statement;
}

std::unique_ptr<If> Parser::do_if(const Token& kw) {
	std::unique_ptr<If> if_statement(new If(kw));
	do_expr(if_statement->expr, "{");
	if_statement->true_block = do_block();
	if (peek().form == Token::KW_ELSE) {
		next();
		const Token& if_tok = next();
		if (if_tok.form == Token::KW_IF) {
			if_statement->else_block.push_back(do_if(if_tok));
		} else if (if_tok.str() == "{"){
			if_statement->else_block = do_block();
		} else {
			throw Exception("Expected 'if' or '{'", if_tok);
		}
	}
	return if_statement;
}

std::unique_ptr<While> Parser::do_while(const Token& kw) {
	std::unique_ptr<While> while_statement(new While(kw));
	if (peek().str() == "{") {
		// of no condition, defaults to infinite loop
		while_statement->expr.emplace_back(new IntTok(kw, true));
	} else {
		do_expr(while_statement->expr, "{");
	}
	while_statement->block = do_block();
	return while_statement;
}

std::unique_ptr<Break> Parser::do_break(const Token& kw) {
	std::unique_ptr<Break> break_statement(new Break(kw));
	const Token& token = next();
	if (token.form != Token::END) {
		// you can do 'return *X' to break out of X loops
		if (token.str() != "*") throw Exception("Expected ';' or '*'", token);
		const Token& amount = next();
		if (amount.form != Token::INT) throw Exception("Expected integer", amount);
		break_statement->amount = (int)amount.i();
		trim();
	}
	return break_statement;
}

void Parser::do_expr(Expr& expr, const std::string& terminator) {
	std::vector<Op*> ops;
	std::vector<const Token*> tokens;
	bool prev_was_op = true;
	bool param_ready = false;
	int num_parameters = 0;
	while (true) {
		const Token& token = peek();
		if (prev_was_op && token.form == Token::END) {
			next();
			continue;
		}
		if (token.form == Token::END || token.str() == "}" || token.str() == terminator) {
			// the expression has terminated
			while (!ops.empty()) {
				if (ops.back() == &PAREN) throw Exception("Unclosed parenthesis", token);
				expr.emplace_back(new FuncTok(*tokens.back(), ops.back()->num_params));
				tokens.pop_back();
				ops.pop_back();
			}
			if (token.str() != "}") next();
			return;
		}
		if (param_ready && peek().str() != ")") {
			num_parameters++;
			param_ready = false;
		}
		bool pwo = prev_was_op;
		prev_was_op = false;
		next();
		switch (token.form) {
			case Token::INT:   expr.emplace_back(new   IntTok(token, token.i(), token.type)); break;
			case Token::FLOAT: expr.emplace_back(new FloatTok(token, token.f(), token.type)); break;
			case Token::KW_TRUE:  expr.emplace_back(new IntTok(token, true)); break;
			case Token::KW_FALSE: expr.emplace_back(new IntTok(token, false)); break;
			case Token::KW_IF: {
				IfTok* tok = new IfTok(token);
				tok->if_statement = do_if(token);
				expr.push_back(std::unique_ptr<IfTok>(tok));
			} break;
			case Token::IDENT:
				if (peek().str() == "(") {
					ops.push_back(&FUNC);
					tokens.push_back(&token);
					num_parameters = 0;
					param_ready = true;
					next();
					ops.push_back(&PAREN);
					tokens.push_back(nullptr);
					prev_was_op = true;
				} else {
					expr.emplace_back(new VarTok(token));
				}
				break;
			case Token::SYMBOL: {
				if (token.str()[0] == ',') {
					param_ready = true;
					prev_was_op = true;
					while (true) {
						if (ops.empty()) throw Exception("Mismatched parenthesis", token);
						Op* op = ops.back();
						if (op == &PAREN) {
							break;
						} else {
							expr.emplace_back(new FuncTok(*tokens.back(), op->num_params));
							ops.pop_back();
							tokens.pop_back();
						}
					}
					break;
				} else if (token.str()[0] == '(') {
					ops.push_back(&PAREN);
					tokens.push_back(nullptr);
					prev_was_op = true;
					break;
				} else if (token.str()[0] == ')') {
					while (true) {
						if (ops.empty()) throw Exception("Mismatched parenthesis", token);
						Op* op = ops.back();
						ops.pop_back();
						if (op == &PAREN) {
							tokens.pop_back();
							if (!ops.empty() && ops.back() == &FUNC) {
								param_ready = false;
								ops.pop_back();
								expr.emplace_back(new FuncTok(*tokens.back(), num_parameters));
								tokens.pop_back();
							}
							break;
						} else {
							expr.emplace_back(new FuncTok(*tokens.back(), op->num_params));
							tokens.pop_back();
						}
					}
					break;
				}

				static std::unordered_map<std::string, Op*> operators = {
					{"+", &ADD}, {"-", &SUB}, {"*", &MUL}, {"/", &DIV}, {"%", &MOD},
					{"<<", &LSH},{">>", &RSH},{"&", &BAND},{"|", &BOR}, {"^", &XOR},
					{"==", &EQ}, {"!=", &NEQ},{"&&", &AND},{"||", &OR},
					{"!", &NOT}, {">", &GT}, {"<", &LT}, {">=", &GEQ}, {"<=", &LEQ}
				};
				auto iter = operators.find(token.str());
				if (iter == operators.end()) throw Exception("Unexpected symbol", token);
				Op* op = iter->second;
				if (op == &SUB && pwo) op = &NEG;
				if (op == &DIV && pwo) op = &INV;
				while (true) {
					if (ops.empty()) break;
					Op* op2 = ops.back();
					if (!((op->left_assoc && op->precidence <= op2->precidence) ||
					      (!op->left_assoc && op->precidence < op2->precidence))) {
						break;
					}
					expr.emplace_back(new FuncTok(*tokens.back(), op2->num_params));
					ops.pop_back();
					tokens.pop_back();
				}
				prev_was_op = true;
				ops.push_back(op);
				tokens.push_back(&token);
			} break;
			default: throw Exception("Unexpected token", token);
		}
	}
}

void Parser::trim() {
	while (peek().form == Token::END) next();
}

const Token& Parser::next(int i) {
	if (index + i - 1 >= tokens->size()) {
		throw Exception("Unterminated block", (*tokens)[index - 1]);
	}
	const Token& res = (*tokens)[index + i - 1];
	index += i;
	return res;
}
const Token& Parser::peek(int i) {
	if (index + i - 1 >= tokens->size()) {
		static Token end(Token::INVALID, "");
		return end;
	}
	return (*tokens)[index + i - 1];
}
void Parser::expect(const std::string& str) {
	const Token& token = next();
	if (token.str() != str) throw Exception("Expected '" + str + "'", token);
}
const Token& Parser::expect_ident() {
	const Token& token = next();
	if (token.form != Token::IDENT) throw Exception("Expected ident", token);
	return token;
}

void Parser::assert_simple_ident(const Token& ident) {
	if (ident.form != Token::IDENT || ident.ident().size() > 1) {
		throw Exception("Expected simple identifier", ident);
	}
}
