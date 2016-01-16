#include "Parser.h"
#include "Except.h"

void Parser::construct(Module& module, const std::vector<Token>& tokens) {
	this->tokens = &tokens;
	index = 0;
	do_module(module);
}

void Parser::do_module(Module& module, bool submodule) {
	while (true) {
		trim();
		if (submodule && peek().str() == "}") break;
		if (index >= tokens->size()) break;
		auto item = do_item(module);
		module.push_back(std::move(item));
	}
}

std::unique_ptr<Item> Parser::do_item(Module& module) {
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
	} else if (token->str() == "struct") {
		item = do_struct(pub, module);
	} else if (token->str() == "module") {
		item = do_submodule(module, false);
	} else if (token->str() == "extend") {
		item = do_submodule(module, true);
	} else {
		throw Except("Expected item", *token);
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
			throw Except("Invalid import syntax", kw);
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
		expect(":");
		function->add_param(*param_token, Type::parse(expect_ident()));
		param_token = &next();
		trim();
		if (param_token->str() == ",") param_token = &next();
		else if (param_token->str() != ")") throw Except("Expected ',' or ')'", *param_token);
		if (param_token->str() == "[") {
			// named parameters
			param_token = &next();
			while (param_token->str() != "]") {
				assert_simple_ident(*param_token);
				expect(":");
				Type type = Type::parse(expect_ident());
				function->add_named_param(*param_token, type);
				param_token = &next();
				if (param_token->str() == "=") {
					Expr expr = do_expr(",");
					function->named_param_vals.back() = eval.eval(type, expr);
					param_token = &peek();
				}
				if (param_token->str() == ",") param_token = &next();
				else if (param_token->str() != "]") throw Except("Expected ']'", *param_token);
			}
			expect(")");
			break;
		}
	}
	assert(function->param_types.size() + function->named_param_names.size() < 256);

	// return type
	const Token* colon_token = &next();
	if (colon_token->str() == ":") {
		const Token& ret_token = expect_ident();
		function->return_type = Type::parse(ret_token);
		colon_token = &next();
	}
	trim();

	if (colon_token->str() != "{") throw Except("Expected ':' or '{'", *colon_token);
	function->block = do_block();
	return std::move(function);
}

std::unique_ptr<Global> Parser::do_global(bool conzt) {
	const Token& name = next();
	expect(":");
	Type type = Type::parse(expect_ident());
	expect("=");
	Expr expr = do_expr();
	return std::unique_ptr<Global>(new Global(name, type, eval.eval(type, expr), conzt));
}

std::unique_ptr<Struct> Parser::do_struct(bool pub, Module& module) {
	const Token& name_token = expect_ident();
	expect("{");
	std::unique_ptr<Struct> strukt(new Struct(name_token));

	const Token* member_token = &next();
	while (member_token->str() != "}") {
		assert_simple_ident(*member_token);
		expect(":");
		strukt->add_member(*member_token, Type::parse(expect_ident()));
		member_token = &next();
		trim();
		if (member_token->str() == ",") member_token = &next();
		else if (member_token->str() != "}") throw Except("Expected ',' or '}'", *member_token);
	}

	Module* submodule = module.create_submodule(name_token.str());
	if (submodule == nullptr) throw Except("Module redeclaration", name_token);
	std::unique_ptr<Item> item(new SubModule(name_token, *submodule));
	item->pub = pub;
	module.push_back(std::move(item));

	Function* constructor = new Function(name_token);
	constructor->pub = pub;
	for (size_t i = 0; i < strukt->member_types.size(); i++) {
		constructor->add_named_param(*strukt->member_names[i], strukt->member_types[i]);
	}
	constructor->return_type = Type(*strukt);
	constructor->form = Function::CONSTRUCTOR;
	strukt->constructor.reset(constructor);
	module.declare(*constructor);

	return std::move(strukt);
}

std::unique_ptr<SubModule> Parser::do_submodule(Module& module, bool extend) {
	const Token& name_token = expect_ident();
	expect("{");
	Module* submodule = extend ? module.search(name_token.ident()) :
	                             module.create_submodule(name_token.str());
	if (submodule == nullptr) {
		throw Except(extend ? "Nonexistant module" : "Module redeclaration", name_token);
	}
	do_module(*submodule, true);
	std::unique_ptr<SubModule> item(new SubModule(name_token, *submodule));
	return std::move(item);
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
					default: throw Except("Expected ':', '=', or an operator", token2);
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
		if (eq_token.str() != "=") throw Except("Expected '='", eq_token);
		declaration.reset(new Declaration(ident, token));
	} else if (token.form == Token::END) {
		declaration.reset(new Declaration(ident));
		return declaration;
	} else {
		throw Except("Expected '=' or identifier.", token);
	}
	do_expr(declaration->expr);
	return declaration;
}

std::unique_ptr<Assignment> Parser::do_assign(const Token& ident, const Token* op_token) {
	std::unique_ptr<Assignment> assignment(new Assignment(ident));
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
			throw Except("Expected 'if' or '{'", if_tok);
		}
	}
	return if_statement;
}

std::unique_ptr<While> Parser::do_while(const Token& kw) {
	std::unique_ptr<While> while_statement(new While(kw));
	if (peek().str() == "{") {
		// of no condition, defaults to infinite loop
		while_statement->expr.emplace_back(new ValueTok(kw, Value(true)));
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
		if (token.str() != "*") throw Except("Expected ';' or '*'", token);
		const Token& amount = next();
		if (amount.form != Token::INT) throw Except("Expected integer", amount);
		break_statement->amount = (int)amount.i();
		trim();
	}
	return break_statement;
}

Expr Parser::do_expr(const std::string& terminator) {
	Expr expr;
	do_expr(expr, terminator);
	return std::move(expr);
}

void Parser::do_expr(Expr& expr, const std::string& terminator) {
	std::vector<Op*> ops;
	std::vector<const Token*> tokens;
	bool prev_was_op  = true;

	bool param_ready  = false;
	int num_args = 0;
	int start_named_args = -1;
	std::vector<std::string> named_args;

	while (true) {
		const Token& token = peek();
		if (prev_was_op && token.form == Token::END) {
			next();
			continue;
		}
		if (token.form == Token::END || token.str() == "}" || token.str() == terminator) {
			// the expression has terminated
			while (!ops.empty()) {
				if (ops.back() == &PAREN) throw Except("Unclosed parenthesis", token);
				expr.emplace_back(new FuncTok(*tokens.back(), ops.back()->num_params));
				tokens.pop_back();
				ops.pop_back();
			}
			if (token.str() != "}") next();
			return;
		}
		if (param_ready && peek().str() != ")") {
			num_args++;
			param_ready = false;
			if (peek().form == Token::IDENT && peek(2).str() == "=") {
				// named parameter
				if (start_named_args == -1) {
					start_named_args = num_args - 1;
				}
				named_args.push_back(peek().str());
				next(2);
				continue;
			} else if (start_named_args != -1) {
				throw Except("Must be named parameters to the end", token);
			}
		}
		bool pwo = prev_was_op;
		prev_was_op = false;
		next();
		switch (token.form) {
			case Token::INT:   expr.emplace_back(new ValueTok(token, Value(token.i()))); break;
			case Token::FLOAT: expr.emplace_back(new ValueTok(token, Value(token.f()))); break;
			case Token::KW_TRUE:  expr.emplace_back(new ValueTok(token, Value(true)));  break;
			case Token::KW_FALSE: expr.emplace_back(new ValueTok(token, Value(false))); break;
			case Token::KW_IF: {
				IfTok* tok = new IfTok(token);
				tok->if_statement = do_if(token);
				expr.push_back(std::unique_ptr<IfTok>(tok));
			} break;
			case Token::IDENT:
				if (peek().str() == "(") {
					// function call
					ops.push_back(&FUNC);
					tokens.push_back(&token);
					num_args = 0;
					start_named_args = -1;
					named_args.clear();
					param_ready = true;
					next();
					ops.push_back(&PAREN);
					tokens.push_back(nullptr);
					prev_was_op = true;
				} else if (peek().str() == "=") {
					// named parameter
				} else {
					expr.emplace_back(new VarTok(token));
				}
				break;
			case Token::SYMBOL: {
				if (token.str()[0] == ',') {
					param_ready = true;
					prev_was_op = true;
					while (true) {
						if (ops.empty()) throw Except("Mismatched parenthesis", token);
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
						if (ops.empty()) throw Except("Mismatched parenthesis", token);
						Op* op = ops.back();
						ops.pop_back();
						if (op == &PAREN) {
							tokens.pop_back();
							if (!ops.empty() && ops.back() == &FUNC) {
								param_ready = false;
								ops.pop_back();
								expr.emplace_back(new FuncTok(*tokens.back(), num_args));
								if (start_named_args != -1) {
									FuncTok& ftok = (FuncTok&) *expr.back();
									ftok.num_unnamed_args = start_named_args;
									ftok.named_args = named_args;
								}
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
				if (iter == operators.end()) throw Except("Unexpected symbol", token);
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
			default: throw Except("Unexpected token", token);
		}
	}
}

void Parser::trim() {
	while (peek().form == Token::END) next();
}

const Token& Parser::next(int i) {
	if (index + i - 1 >= tokens->size()) {
		throw Except("Unterminated block", (*tokens)[index - 1]);
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
	if (token.str() != str) throw Except("Expected '" + str + "'", token);
}
const Token& Parser::expect_ident() {
	trim();
	const Token& token = next();
	if (token.form != Token::IDENT) throw Except("Expected ident", token);
	return token;
}

void Parser::assert_simple_ident(const Token& ident) {
	if (ident.form != Token::IDENT || ident.ident().size() > 1) {
		throw Except("Expected simple identifier", ident);
	}
}
