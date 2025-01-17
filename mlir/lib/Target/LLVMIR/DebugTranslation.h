//===- DebugTranslation.h - MLIR to LLVM Debug conversion -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the translation between an MLIR debug information and
// the corresponding LLVMIR representation.
//
//===----------------------------------------------------------------------===//

#ifndef MLIR_LIB_TARGET_LLVMIR_DEBUGTRANSLATION_H_
#define MLIR_LIB_TARGET_LLVMIR_DEBUGTRANSLATION_H_

#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/Location.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/DIBuilder.h"

namespace mlir {
class Operation;

namespace LLVM {
class LLVMFuncOp;

namespace detail {
class DebugTranslation {
public:
  DebugTranslation(Operation *module, llvm::Module &llvmModule);

  /// Finalize the translation of debug information.
  void finalize();

  /// Translate the given location to an llvm debug location.
  llvm::DILocation *translateLoc(Location loc, llvm::DILocalScope *scope);

  /// Translates the given DWARF expression metadata to to LLVM.
  llvm::DIExpression *translateExpression(LLVM::DIExpressionAttr attr);

  /// Translates the given DWARF global variable expression to LLVM.
  llvm::DIGlobalVariableExpression *
  translateGlobalVariableExpression(LLVM::DIGlobalVariableExpressionAttr attr);

  /// Translate the debug information for the given function.
  void translate(LLVMFuncOp func, llvm::Function &llvmFunc);

  /// Translate the given LLVM debug metadata to LLVM.
  llvm::DINode *translate(DINodeAttr attr);

  /// Translate the given derived LLVM debug metadata to LLVM.
  template <typename DIAttrT>
  auto translate(DIAttrT attr) {
    // Infer the LLVM type from the attribute type.
    using LLVMTypeT = std::remove_pointer_t<decltype(translateImpl(attr))>;
    return cast_or_null<LLVMTypeT>(translate(DINodeAttr(attr)));
  }

private:
  /// Translate the given location to an llvm debug location with the given
  /// scope and inlinedAt parameters.
  llvm::DILocation *translateLoc(Location loc, llvm::DILocalScope *scope,
                                 llvm::DILocation *inlinedAt);

  /// Create an llvm debug file for the given file path.
  llvm::DIFile *translateFile(StringRef fileName);

  /// Translate the given attribute to the corresponding llvm debug metadata.
  llvm::DIType *translateImpl(DINullTypeAttr attr);
  llvm::DIBasicType *translateImpl(DIBasicTypeAttr attr);
  llvm::DICompileUnit *translateImpl(DICompileUnitAttr attr);
  llvm::DICompositeType *translateImpl(DICompositeTypeAttr attr);
  llvm::DIDerivedType *translateImpl(DIDerivedTypeAttr attr);
  llvm::DIFile *translateImpl(DIFileAttr attr);
  llvm::DILabel *translateImpl(DILabelAttr attr);
  llvm::DILexicalBlock *translateImpl(DILexicalBlockAttr attr);
  llvm::DILexicalBlockFile *translateImpl(DILexicalBlockFileAttr attr);
  llvm::DILocalScope *translateImpl(DILocalScopeAttr attr);
  llvm::DILocalVariable *translateImpl(DILocalVariableAttr attr);
  llvm::DIGlobalVariable *translateImpl(DIGlobalVariableAttr attr);
  llvm::DIModule *translateImpl(DIModuleAttr attr);
  llvm::DINamespace *translateImpl(DINamespaceAttr attr);
  llvm::DIScope *translateImpl(DIScopeAttr attr);
  llvm::DISubprogram *translateImpl(DISubprogramAttr attr);
  llvm::DISubrange *translateImpl(DISubrangeAttr attr);
  llvm::DISubroutineType *translateImpl(DISubroutineTypeAttr attr);
  llvm::DIType *translateImpl(DITypeAttr attr);

  /// Attributes that support self recursion need to implement two methods and
  /// hook into the `translateImpl` overload for `DIRecursiveTypeAttr`.
  /// - `<llvm type> translateImplGetPlaceholder(<mlir type>)`:
  ///   Translate the DI attr without translating any potentially recursive
  ///   nested DI attrs.
  /// - `void translateImplFillPlaceholder(<mlir type>, <llvm type>)`:
  ///   Given the placeholder returned by `translateImplGetPlaceholder`, fill
  ///   any holes by recursively translating nested DI attrs. This method must
  ///   mutate the placeholder that is passed in, instead of creating a new one.
  llvm::DIType *translateRecursive(DIRecursiveTypeAttrInterface attr);

  /// Get a placeholder DICompositeType without recursing into the elements.
  llvm::DICompositeType *translateImplGetPlaceholder(DICompositeTypeAttr attr);
  /// Completes the DICompositeType `placeholder` by recursively translating the
  /// elements.
  void translateImplFillPlaceholder(DICompositeTypeAttr attr,
                                    llvm::DICompositeType *placeholder);

  /// Constructs a string metadata node from the string attribute. Returns
  /// nullptr if `stringAttr` is null or contains and empty string.
  llvm::MDString *getMDStringOrNull(StringAttr stringAttr);

  /// A mapping between mlir location+scope and the corresponding llvm debug
  /// metadata.
  DenseMap<std::tuple<Location, llvm::DILocalScope *, const llvm::DILocation *>,
           llvm::DILocation *>
      locationToLoc;

  /// A mapping between debug attribute and the corresponding llvm debug
  /// metadata.
  DenseMap<Attribute, llvm::DINode *> attrToNode;

  /// A mapping between recursive ID and the translated DIType.
  /// DIType.
  llvm::MapVector<DistinctAttr, llvm::DIType *> recursiveTypeMap;

  /// A mapping between a distinct ID and the translated LLVM metadata node.
  /// This helps identify attrs that should translate into the same LLVM debug
  /// node.
  DenseMap<DistinctAttr, llvm::DINode *> distinctAttrToNode;

  /// A mapping between filename and llvm debug file.
  /// TODO: Change this to DenseMap<Identifier, ...> when we can
  /// access the Identifier filename in FileLineColLoc.
  llvm::StringMap<llvm::DIFile *> fileMap;

  /// A string containing the current working directory of the compiler.
  SmallString<256> currentWorkingDir;

  /// Flag indicating if debug information should be emitted.
  bool debugEmissionIsEnabled;

  /// Debug information fields.
  llvm::Module &llvmModule;
  llvm::LLVMContext &llvmCtx;
};

} // namespace detail
} // namespace LLVM
} // namespace mlir

#endif // MLIR_LIB_TARGET_LLVMIR_DEBUGTRANSLATION_H_
