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

#include "tools/string_utils.h"
#include "base/array.h"
#include "base/vector.h"

#include <iostream>
#include <set>
#include <fstream>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory MatcherSampleCategory("Matcher Sample");


std::string BuildTemplateParamString(const TemplateParameterList * temp_params) {
  std::string temp_param_string = "";
  if (temp_params && temp_params->getMinRequiredArguments() > 0) {
    
    //std::cout << temp_params->getParam(0)->getDeclKindName() << " " << temp_params->getParam(0)->getNameAsString() << std::endl;
    if (!strcmp(temp_params->getParam(0)->getDeclKindName(), "TemplateTypeParm")) {
      temp_param_string += "int";
    } else {
      temp_param_string += "1";
    }

    for (size_t i = 1; i < temp_params->getMinRequiredArguments(); i++) {
      //std::cout << i << temp_param_string<< " "<< temp_params->getParam(i)->getDeclKindName() << " " << temp_params->getParam(i)->getNameAsString() << std::endl;
      if (!strcmp(temp_params->getParam(i)->getDeclKindName(), "TemplateTypeParm")) {
	temp_param_string += ",int";
      } else {
	temp_param_string += ",1";
      }  
    }
  }
  return temp_param_string;
}

std::map<std::string, std::string> BuildTemplateParamMap(const TemplateParameterList * temp_params) {

  std::map<std::string, std::string> param_map;

  if (temp_params) {

    for (const NamedDecl * p : *temp_params) {
      // std::cout << " "<< p->getDeclKindName() << " " << p->getNameAsString() << std::endl;
      if (!strcmp(p->getDeclKindName(), "TemplateTypeParm")) {
	      param_map[p->getNameAsString()] = "int";
      } else {
      	param_map[p->getNameAsString()] = "1";
      }  
    }

    //if (temp_params->containsUnexpandedParameterPack()) {
    //  std::prev(temp_params.end())
    //}
  }
  return param_map;
}

std::string TranslateTemplateArg(std::map<std::string, std::string> & map, const TemplateParameterList * temp_list) {

  std::cout << "Entering translate" << std::endl;
  std::string temp_string = "";
  for (const NamedDecl * p : *temp_list) {
    if (map.find(p->getNameAsString()) != map.end()) {
      temp_string += map[p->getNameAsString()];
    } else if(p->getDescribedTemplate()) {
      temp_string += "<" + TranslateTemplateArg(map, p->getDescribedTemplate()->getTemplateParameters()) + ">";
    } else {
      temp_string  += p->getNameAsString();
    }
  }

   std::cout << "Exiting translate" << std::endl;
  return temp_string;
}

std::string TranslateIndividualArg(std::string in_str, std::map<std::string, std::string> & param_map) {
    emp::remove_whitespace(in_str);
    if (param_map.find(in_str) != param_map.end()) {
      return param_map[in_str];
    } else if (in_str.find(",") != std::string::npos) {
      emp::vector<std::string> sliced_vec = emp::slice(in_str, ',');
      std::string result = TranslateIndividualArg(sliced_vec[0], param_map);
      for (size_t i = 1; i < sliced_vec.size(); i++) {
        result += "," + TranslateIndividualArg(sliced_vec[1], param_map);
      }
      return result;
    } else {
      return in_str;
    }

}

std::string ParseTemplateArgs(std::string in_str, std::map<std::string, std::string> & param_map) {

  std::cout << "Parsing: " << in_str << std::endl;
  emp::vector<emp::array<size_t, 2> > ranges;
  emp::vector<size_t> starts;
  size_t last_end = 0;
  for (size_t i = 0; i < in_str.size(); i++) {
    std::cout << "Starts: " << emp::to_string(starts) << std::endl;
    if (in_str[i] == '<') {
      starts.push_back(i);
      if (starts.size() == 1 && i > 0) {
        ranges.emplace_back(emp::array<size_t, 2>({last_end, i-1}));
      }
    } else if (in_str[i] == '>') {
      emp_assert(starts.size() > 0);
      size_t s = starts.back();
      starts.pop_back();
      if (starts.size() == 0) {
        ranges.emplace_back(emp::array<size_t, 2>({s, i}));
        last_end = i+1;
      }
    }
  }
  if (last_end != in_str.size()) {
    ranges.emplace_back(emp::array<size_t, 2>({last_end, in_str.size()-1}));
  }
  std::cout << "ranges: "<< emp::to_string(ranges)<< std::endl;
  emp_assert(starts.size() == 0);

  if (ranges.size() == 1 && in_str[0] != '<') {
    return TranslateIndividualArg(in_str, param_map);
  } else if (ranges.size() == 1) {
    std::cout << "removing braces" << std::endl;
    std::cout << in_str.substr(1, in_str.size()-2) << std::endl;
    in_str = in_str.substr(1, in_str.size()-2);
    return "<" + TranslateIndividualArg(in_str, param_map) + ">";
  }

  std::string result = "";
  for (auto & range : ranges) {
    std::cout << "About to recurse: " << emp::to_string(range) << " " << in_str.substr(range[0], range[1]-range[0]+1)  << std::endl;
    result += ParseTemplateArgs(in_str.substr(range[0], range[1]-range[0]+1), param_map);
  }

  return result;
}

std::string BuildArgString(const FunctionDecl * fun, std::map<std::string, std::string> & param_map) {


  std::string param_string = "";

  if (fun && fun->getMinRequiredArguments() > 0) {
    
    //std::cout << fun->getParamDecl(0)->getDeclKindName() << " " << fun->getParamDecl(0)->getNameAsString() << std::endl;

    if (param_map.find(fun->getParamDecl(0)->getType().getAsString()) != param_map.end()) {
      // std::cout << "in map" << std::endl;
      param_string += param_map[fun->getParamDecl(0)->getType().getAsString()];
    } else {
      std::cout << "decomposing" << std::endl;
      param_string += ParseTemplateArgs(fun->getParamDecl(0)->getType().getAsString(), param_map);
    } 
    
    for (size_t i = 1; i < fun->getMinRequiredArguments(); i++) {
      
      std::cout << "Incremental param string: " << param_string  << std::endl;
      //std::cout << i << " " << fun->getParamDecl(i)->getDeclKindName() << " " << fun->getParamDecl(i)->getNameAsString() << std::endl;
      if (param_map.find(fun->getParamDecl(i)->getType().getAsString()) != param_map.end()) {
	      std::cout << "in map" << std::endl;
	      param_string += "," + param_map[fun->getParamDecl(i)->getType().getAsString()];
      } else {
        std::cout << "decomposing" << std::endl;
        param_string += ", " + ParseTemplateArgs(fun->getParamDecl(i)->getType().getAsString(), param_map);
      } 
    }

  }
  std::cout << "Final param string " << param_string << std::endl; 
  return param_string;
}


class MethodHandler : public MatchFinder::MatchCallback {
public:
  MethodHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    const CXXMethodDecl *MethodNode = Result.Nodes.getNodeAs<CXXMethodDecl>("method");
    
    if (!MethodNode->getReturnType().getTypePtr()->getContainedAutoType() && MethodNode->isFirstDecl() && !MethodNode->isStatic() && MethodNode->getLocation().isValid() && !MethodNode->getTemplatedKind()) {
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
    
    if (ClassNode->isImplicit() || !ClassNode->getLocation().isValid()) {
      return;
    }

    std::string filename = Rewrite.getSourceMgr().getFilename(ClassNode->getSourceRange().getBegin()).str()+"template_instantiations.h";
    declfile.open(filename, std::ios::app);
    if (!declfile.tellp()) {
      declfile << "#include " << filename << ";" << std::endl;
    }

    std::string temp_param_string = BuildTemplateParamString(ClassNode->getTemplateParameters());
    
    declfile << "template class " + ClassNode->getQualifiedNameAsString() + "<" + temp_param_string + ">;" << std::endl;
    declfile.close();
  }

private:
  Rewriter &Rewrite;
  std::ofstream declfile;
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
  TemplateFunctionHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    const FunctionTemplateDecl *FunctionNode = Result.Nodes.getNodeAs<FunctionTemplateDecl>("fun");
    const FunctionDecl * fun = FunctionNode->getAsFunction();

    if (!fun || FunctionNode->isImplicit() || !FunctionNode->getLocation().isValid()) {
      return;
    }
    
    const TemplateParameterList * temp_params = FunctionNode->getTemplateParameters();

    std::string filename = Rewrite.getSourceMgr().getFilename(FunctionNode->getSourceRange().getBegin()).str();
    declfile.open(filename + "template_instantiations.h", std::ios::app);
    if (!declfile.tellp()) {
      declfile << "#include " << filename << ";" << std::endl;
    }
    
    std::set<std::string> temp_param_strings;
    std::map<std::string, std::string> param_map = BuildTemplateParamMap(temp_params);
    std::string param_string = BuildArgString(fun, param_map);


    std::string ret_type = fun->getReturnType().getAsString();
    if (param_map.find(ret_type) != param_map.end()) {
      ret_type = param_map[ret_type];
    }

    std::string temp_param_string = BuildTemplateParamString(FunctionNode->getTemplateParameters());
      
    //Rewrite.InsertText(Rewrite.getSourceMgr().getLocForEndOfFile(), "template " +
    //	       ret_type + " " +
    //	       FunctionNode->getNameAsString()+ "<" + temp_param_string +
    //	       ">(" + param_string + ");\n", true, true);

    declfile << "template " + ret_type + " " + FunctionNode->getQualifiedNameAsString()+ "<" + temp_param_string + ">(" + param_string + ");\n" << std::endl;
    declfile.close();

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


    Matcher.addMatcher(cxxMethodDecl(unless(anyOf(cxxConstructorDecl(), isImplicit(), isVirtual(), isConstexpr()))).bind("method"), &HandlerForMethod);
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
