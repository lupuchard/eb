#include "Builder.h"
#include "llvm/IR/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include <fstream>
#include <bits/stl_set.h>
#include <iostream>

std::string Builder::build(Module& module, State& state) {
	std::string out;
	llvm::raw_string_ostream stream(out);
	build(module, state, stream);
	return stream.str();
}

void Builder::build(Module& module, State& state, const std::string& out_file) {
	std::ofstream file;
	file.open(out_file);
	llvm::raw_os_ostream stream(file);
	build(module, state, stream);
}

void Builder::build(Module& module, State& state, llvm::raw_ostream& stream) {
	llvm::Module llvm_module("thang_main", llvm::getGlobalContext());
	c = &llvm_module.getContext();
	do_module(module, llvm_module, state);
	llvm::PassManager pass_manager;
	pass_manager.add(llvm::createPrintModulePass(&stream));
	pass_manager.run(llvm_module);
}

llvm::Type* Builder::type_to_llvm(Type type) {
	switch (type) {
		case Type::BOOL: return llvm::Type::getInt1Ty(*c);
		case Type::I8:   return llvm::Type::getInt8Ty(*c);
		case Type::I16:  return llvm::Type::getInt16Ty(*c);
		case Type::I32:  return llvm::Type::getInt32Ty(*c);
		case Type::I64:  return llvm::Type::getInt64Ty(*c);
		case Type::U8:   return llvm::Type::getInt8Ty(*c);
		case Type::U16:  return llvm::Type::getInt16Ty(*c);
		case Type::U32:  return llvm::Type::getInt32Ty(*c);
		case Type::U64:  return llvm::Type::getInt64Ty(*c);
		case Type::F32:  return llvm::Type::getFloatTy(*c);
		case Type::F64:  return llvm::Type::getDoubleTy(*c);
		default: assert(false);
	}
}

void Builder::do_module(Module& module, llvm::Module& llvm_module, State& state) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = *module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;

				llvm::Type* ret_type = type_to_llvm(func.return_type);
				std::vector<llvm::Type*> params;
				for (size_t j = 0; j < func.param_types.size(); j++) {
					params.push_back(type_to_llvm(func.param_types[j]));
				}

				std::string name = func.name_token.str;
				name = name == "main" ? "eb$main" : name;
				llvm_func = llvm::cast<llvm::Function>(llvm_module.getOrInsertFunction(name,
						llvm::FunctionType::get(ret_type, llvm::ArrayRef<llvm::Type*>(params), false)
				));
				state.set_func_llvm(name, *llvm_func);

				state.descend(func.block);
				auto iter = llvm_func->arg_begin();
				for (size_t j = 0; j < func.param_types.size(); j++) {
					state.get_var(func.param_names[j]->str)->llvm = &*iter++;
				}
				llvm::IRBuilder<> builder(create_basic_block("entry"));

				do_block(builder, func.block, state);
				state.ascend();
			} break;
		}
	}
}

// returns whether or not the block ended on a return
bool Builder::do_block(llvm::IRBuilder<>& builder, Block& block, State& state) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		switch (statement.form) {
			case Statement::DECLARATION: {
				Declaration& decl = (Declaration&)statement;
				Variable& var = state.next_var(decl.token.str);
				auto llvm_type = type_to_llvm(var.type);
				auto llvm_val = builder.CreateAlloca(llvm_type, nullptr, decl.token.str);
				var.llvm = llvm_val;
				llvm::Value* assigned;
				if (decl.value == nullptr) {
					assigned = default_value(var.type, llvm_type);
				} else {
					assigned = do_expr(builder, *decl.value, state);
				}
				builder.CreateStore(assigned, llvm_val);
			} break;
			case Statement::ASSIGNMENT: {
				Assignment& assignment = (Assignment&)statement;
				llvm::Value* assigned = do_expr(builder, *assignment.value, state);
				builder.CreateStore(assigned, state.get_var(assignment.token.str)->llvm);
			} break;
			case Statement::RETURN: {
				Return& return_statement = (Return&)statement;
				llvm::Value* returned = do_expr(builder, *return_statement.value, state);
				builder.CreateRet(returned);
				return true;
			}
			case Statement::IF: {
				If& if_statement = (If&)statement;

				llvm::BasicBlock* end = create_basic_block("endif");
				llvm::BasicBlock* if_false = nullptr;

				bool exits = false;

				for (size_t j = 0; j < if_statement.conditions.size(); j++) {
					llvm::Value* cond = do_expr(builder, *if_statement.conditions[j], state);
					llvm::BasicBlock* if_true = create_basic_block("if");
					if_false = create_basic_block("else");
					builder.CreateCondBr(cond, if_true, if_false);

					builder.SetInsertPoint(if_true);
					Block& if_block = if_statement.blocks[j];
					state.descend(if_block);
					if (!do_block(builder, if_block, state)) {
						builder.CreateBr(end);
						exits = true;
					}
					state.ascend();

					builder.SetInsertPoint(if_false);
				}

				if (if_statement.conditions.size() < if_statement.blocks.size()) {
					Block& else_block = if_statement.blocks.back();
					state.descend(else_block);
					if (!do_block(builder, else_block, state)) {
						builder.CreateBr(end);
						exits = true;
					}
					state.ascend();
				} else {
					builder.CreateBr(end);
					exits = true;
				}

				if (exits) {
					end->moveAfter(if_false);
					builder.SetInsertPoint(end);
				} else {
					end->eraseFromParent();
				}
			} break;
			case Statement::WHILE: {
				While& while_statement = (While&)statement;
				llvm::BasicBlock* start   = create_basic_block("start");
				llvm::BasicBlock* if_true = create_basic_block("loop");
				llvm::BasicBlock* end     = create_basic_block("end");
				builder.CreateBr(start);
				builder.SetInsertPoint(start);
				llvm::Value* cond = do_expr(builder, *while_statement.condition, state);
				builder.CreateCondBr(cond, if_true, end);
				builder.SetInsertPoint(if_true);
				state.descend(while_statement.block);
				Loop& loop = *state.get_loop(1);
				loop.start = start;
				loop.end   = end;
				do_block(builder, while_statement.block, state);
				state.ascend();
				builder.CreateBr(start);
				builder.SetInsertPoint(end);
			} break;
			case Statement::CONTINUE:
				builder.CreateBr(state.get_loop(1)->start);
				return true;
			case Statement::BREAK: {
				Break& break_statement = (Break&)statement;
				builder.CreateBr(state.get_loop(break_statement.amount)->end);
				return true;
			}
		}
	}
	return false;
}

llvm::Value* Builder::do_expr(llvm::IRBuilder<>& builder, Expr& expr, State& state) {
	std::vector<llvm::Value*> value_stack;
	std::vector<Type> type_stack;
	for (Tok& tok : expr) {
		switch (tok.form) {
			case Tok::INT_LIT:
				if (is_type(tok.type, Type::FLOAT)) {
					value_stack.push_back(llvm::ConstantFP::get(type_to_llvm(tok.type), tok.i));
				} else {
					value_stack.push_back(llvm::ConstantInt::get(type_to_llvm(tok.type), tok.i));
				}
				type_stack.push_back(tok.type);
				break;
			case Tok::FLOAT_LIT:
				value_stack.push_back(llvm::ConstantFP::get(type_to_llvm(tok.type), tok.f));
				type_stack.push_back(tok.type);
				break;
			case Tok::BOOL_LIT:
				value_stack.push_back(llvm::ConstantInt::get(type_to_llvm(tok.type), tok.i));
				type_stack.push_back(tok.type);
				break;
			case Tok::VAR: {
				Variable& var = *state.get_var(tok.token.str);
				if (var.is_param) {
					value_stack.push_back(var.llvm);
				} else {
					value_stack.push_back(builder.CreateLoad(var.llvm, tok.token.str.c_str()));
				}
				type_stack.push_back(var.type);
			} break;
			case Tok::FUNCTION: {
				Function& func = *state.get_func(tok.token.str);
				std::vector<llvm::Value*> args;
				args.resize(func.param_types.size());
				for (int i = (int)func.param_types.size() - 1; i >= 0; i--) {
					args[i] = value_stack.back();
					value_stack.pop_back();
					type_stack.pop_back();
				}
				value_stack.push_back(builder.CreateCall(
						state.get_func_llvm(tok.token.str),
						llvm::ArrayRef<llvm::Value*>(args), func.name_token.str
				));
				type_stack.push_back(func.return_type);
			} break;
			case Tok::OP:
				do_op(builder, tok.op, tok.type, value_stack, type_stack);
				break;
		}
	}
	return value_stack.back();
}

llvm::Constant* Builder::default_value(Type type, llvm::Type* llvm_type) {
	if (is_type(type, Type::FLOAT)) {
		return llvm::ConstantFP::get(llvm_type, 0);
	} else {
		return llvm::ConstantInt::get(llvm_type, 0);
	}
}

bool is_binary(Op op) {
	switch (op) {
		case Op::NEG: case Op::INV: case Op::NOT: return false;
		default: return true;
	}
}
void Builder::do_op(llvm::IRBuilder<>& builder, Op op, Type result_type,
                    std::vector<llvm::Value*>& value_stack, std::vector<Type>& type_stack) {
	llvm::Value* res;
	if (is_binary(op)) {
		auto b = value_stack.back(); value_stack.pop_back();
		type_stack.pop_back();
		auto a = value_stack.back(); value_stack.pop_back();
		Type type = type_stack.back(); type_stack.pop_back();
		switch (op) {
			case Op::ADD: res = is_type(type, Type::FLOAT)  ? builder.CreateFAdd(a, b) :
		                        is_type(type, Type::SIGNED) ? builder.CreateNSWAdd(a, b) :
		                                                      builder.CreateNUWAdd(a, b); break;
			case Op::SUB: res = is_type(type, Type::FLOAT)  ? builder.CreateFSub(a, b) :
			                    is_type(type, Type::SIGNED) ? builder.CreateNSWSub(a, b) :
			                                                  builder.CreateNUWSub(a, b); break;
			case Op::MUL: res = is_type(type, Type::FLOAT)  ? builder.CreateFMul(a, b) :
			                    is_type(type, Type::SIGNED) ? builder.CreateNSWMul(a, b) :
			                                                  builder.CreateNUWMul(a, b); break;
			case Op::DIV: res = is_type(type, Type::FLOAT)  ? builder.CreateFDiv(a, b) :
			                    is_type(type, Type::SIGNED) ? builder.CreateSDiv(a, b) :
			                                                  builder.CreateUDiv(a, b); break;
			case Op::MOD: res = is_type(type, Type::FLOAT)  ? builder.CreateFRem(a, b) :
			                    is_type(type, Type::SIGNED) ? builder.CreateSRem(a, b) :
			                                                  builder.CreateURem(a, b); break;
			case Op::BAND: case Op::AND: res = builder.CreateAnd( a, b); break;
			case Op::BOR:  case Op::OR:  res = builder.CreateOr(  a, b); break;
			case Op::XOR:                res = builder.CreateXor( a, b); break;
			case Op::LSH:                res = builder.CreateShl( a, b); break;
			case Op::RSH:                res = builder.CreateAShr(a, b); break;
			case Op::EQ:  res = is_type(type, Type::FLOAT)  ? builder.CreateFCmpUEQ(a, b) :
		                                                      builder.CreateICmpEQ(a, b); break;
			case Op::NEQ: res = is_type(type, Type::FLOAT)  ? builder.CreateFCmpUNE(a, b) :
		                                                      builder.CreateICmpNE(a, b); break;
			case Op::GT:  res = is_type(type, Type::FLOAT)  ? builder.CreateFCmpUGT(a, b) :
			                    is_type(type, Type::SIGNED) ? builder.CreateICmpSGT(a, b) :
			                                                  builder.CreateICmpUGT(a, b); break;
			case Op::LT:  res = is_type(type, Type::FLOAT)  ? builder.CreateFCmpULT(a, b) :
			                    is_type(type, Type::SIGNED) ? builder.CreateICmpSLT(a, b) :
			                                                  builder.CreateICmpULT(a, b); break;
			case Op::GEQ: res = is_type(type, Type::FLOAT)  ? builder.CreateFCmpUGE(a, b) :
			                    is_type(type, Type::SIGNED) ? builder.CreateICmpSGE(a, b) :
			                                                  builder.CreateICmpUGE(a, b); break;
			case Op::LEQ: res = is_type(type, Type::FLOAT)  ? builder.CreateFCmpULE(a, b) :
			                    is_type(type, Type::SIGNED) ? builder.CreateICmpSLE(a, b) :
			                                                  builder.CreateICmpULE(a, b); break;
			default: assert(false);
		}
	} else {
		auto a = value_stack.back(); value_stack.pop_back();
		Type type = type_stack.back(); type_stack.pop_back();
		switch (op) {
			case Op::NOT: res = builder.CreateNot(a); break;
			case Op::NEG: res = is_type(type, Type::FLOAT)  ? builder.CreateFNeg(a) :
		                        is_type(type, Type::SIGNED) ? builder.CreateNSWNeg(a) :
		                                                      builder.CreateNUWNeg(a); break;
			case Op::INV: res = is_type(type, Type::FLOAT)  ? builder.CreateFDiv(llvm::ConstantFP::get(a->getType(), 0), a) :
		                        is_type(type, Type::SIGNED) ? builder.CreateSDiv(llvm::ConstantInt::get(a->getType(), 0), a) :
		                                                      builder.CreateUDiv(llvm::ConstantInt::get(a->getType(), 0), a); break;
			default: assert(false); break;
		}
	}
	value_stack.push_back(res);
	type_stack.push_back(result_type);
}

llvm::BasicBlock* Builder::create_basic_block(std::string name) {
	return llvm::BasicBlock::Create(*c, name, llvm_func);
}
