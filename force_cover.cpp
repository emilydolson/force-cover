//------------------------------------------------------------------------------
// Force test coverage in C++
//
// This code modifies C++ source code to force test coverage
//
//
// Adapted from AST matching sample by
// Eli Bendersky (eliben@gmail.com)
//------------------------------------------------------------------------------
#include <string>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/Decl.h"

#include <iostream>
#include <set>
#include <fstream>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory MatcherSampleCategory("Matcher Sample");


class MethodHandler : public MatchFinder::MatchCallback {
public:
  MethodHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    const CXXMethodDecl *MethodNode = Result.Nodes.getNodeAs<CXXMethodDecl>("method");
    
    if (MethodNode->getLocation().isValid() && !MethodNode->getTemplatedKind()) {
      Rewrite.InsertText(MethodNode->getLocStart(), "virtual ", true, true);
    }
  }

private:
  Rewriter &Rewrite;
};

class TemplateClassHandler : public MatchFinder::MatchCallback {
public:
  TemplateClassHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    const ClassTemplateDecl *ClassNode = Result.Nodes.getNodeAs<ClassTemplateDecl>("class");

    const TemplateParameterList * temp_params = ClassNode->getTemplateParameters();

    std::string temp_param_string = "";
    if (temp_params && temp_params->getMinRequiredArguments() > 0) {
      
      temp_param_string += "int";

      for (size_t i = 0; i < temp_params->getMinRequiredArguments()-1; i++) {
	temp_param_string += ",int";
      }

    }


    
    Rewrite.InsertText(ClassNode->getLocEnd().getLocWithOffset(2), "template class " + ClassNode->getNameAsString() + "<" + temp_param_string + ">;", true, true);

  }

private:
  Rewriter &Rewrite;
};

class InlineFunctionHandler : public MatchFinder::MatchCallback {
public:
  InlineFunctionHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    const FunctionDecl *FunNode = Result.Nodes.getNodeAs<FunctionDecl>("fun");
    //FunNode->setInlineSpecified(false);

    //if (FunNode->isInlineSpecified()) {
    //  std::cout << FunNode->getNameAsString() << " " << FunNode->getQualifiedNameAsString()<< std::endl;
    //}
    
    //Rewrite.InsertText(ClassNode->getLocEnd().getLocWithOffset(2), "template class " + ClassNode->getNameAsString() + "<int>;", true, true);

  }

private:
  Rewriter &Rewrite;
};


class TemplateFunctionHandler : public MatchFinder::MatchCallback {
public:
  TemplateFunctionHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {
    declfile.open("template_instantiations.h");
  }

  virtual void run(const MatchFinder::MatchResult &Result) {
    const FunctionTemplateDecl *FunctionNode = Result.Nodes.getNodeAs<FunctionTemplateDecl>("fun");
    const FunctionDecl * fun = FunctionNode->getAsFunction();
    const TemplateParameterList * temp_params = FunctionNode->getTemplateParameters();

    std::set<std::string> temp_param_strings;
    std::string temp_param_string = "";
    if (temp_params) {
      for (const NamedDecl * p : *temp_params) {
	temp_param_strings.insert(p->getNameAsString());
      }

      if (temp_param_strings.size() > 0) {
	temp_param_string += "int";
      }
      for (size_t i = 0; i < temp_param_strings.size()-1; i++) {
	temp_param_string += ",int";
      }
    }

    std::string param_string = "";
    if (fun) {
      for (size_t i = 0; i < fun->getNumParams(); i++) {
	const ParmVarDecl * params = fun->getParamDecl(i);
	if (params) {
	  std::string par_type = params->getType().getAsString();
	  if (temp_param_strings.find(par_type) != temp_param_strings.end()) {
	    par_type = "int";
	  }
	  param_string += par_type;
	  //param_string += " ";
	  //param_string += params->getNameAsString();
	}
	
      }

      std::string ret_type = fun->getReturnType().getAsString();
      if (temp_param_strings.find(ret_type) != temp_param_strings.end()) {
	ret_type = "int";
      }

      std::string class_name = "";
      if (dynamic_cast<const CXXMethodDecl*>(fun)) {
	class_name = dynamic_cast<const CXXMethodDecl*>(fun)->getParent()->getNameAsString() + "::";
      }
      
      //Rewrite.InsertText(Rewrite.getSourceMgr().getLocForEndOfFile(), "template " +
      //	       ret_type + " " +
      //	       FunctionNode->getNameAsString()+ "<" + temp_param_string +
      //	       ">(" + param_string + ");\n", true, true);

      declfile << "template " + ret_type + " " + class_name+ FunctionNode->getNameAsString()+ "<" + temp_param_string +
	">(" + param_string + ");\n" << std::endl;

    }

  }

private:
  Rewriter Rewrite;
  std::ofstream declfile;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser. It registers a couple of matchers and runs them on
// the AST.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : HandlerForMethod(R), HandlerForTemplateClass(R),  HandlerForTemplateFunction(R), HandlerForInlineFunction(R) {

    // Add a complex matcher for finding 'for' loops with an initializer set
    // to 0, < comparison in the codition and an increment. For example:
    //
    //  for (int i = 0; i < N; ++i)


    Matcher.addMatcher(cxxMethodDecl(unless(anyOf(cxxConstructorDecl(), isImplicit(), isVirtual()))).bind("method"), &HandlerForMethod);
    Matcher.addMatcher(classTemplateDecl().bind("class"), &HandlerForTemplateClass);
    Matcher.addMatcher(functionTemplateDecl().bind("fun"), &HandlerForTemplateFunction);
    Matcher.addMatcher(functionDecl(isInline()).bind("fun"), &HandlerForInlineFunction);
  }

  void HandleTranslationUnit(ASTContext &Context) override {
    // Run the matchers when we have the whole TU parsed.
    Matcher.matchAST(Context);
  }

private:
  MethodHandler HandlerForMethod;
  TemplateClassHandler HandlerForTemplateClass;
  TemplateFunctionHandler HandlerForTemplateFunction;
  InlineFunctionHandler HandlerForInlineFunction;
  
  MatchFinder Matcher;
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID())
        .write(llvm::outs());
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, MatcherSampleCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
