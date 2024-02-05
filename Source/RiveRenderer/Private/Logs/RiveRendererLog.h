// Copyright Rive, Inc. All rights reserved.

#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogRiveRenderer, VeryVerbose, All);

class FDebugLogger
{
private:
	const int SpacesPerIndent = 2;
	int IndentLevel = 0;
	FString IndentStr{};
public:
	int CurrentLevel() const
	{
		return IndentLevel;
	}
	const FString& CurrentIndent() const
	{
		return IndentStr;
	}
	void Indent()
	{
		++IndentLevel;
		IndentStr.Append(TEXT(".")).Append(TEXT("          "), SpacesPerIndent);
	}
	void UnIndent()
	{
		--IndentLevel;
		IndentStr.RemoveAt(IndentStr.Len() - SpacesPerIndent - 2, SpacesPerIndent + 1);
	}
	static FDebugLogger& Get()
	{
		static FDebugLogger Logger = {};
		return Logger;
	}

	static const TCHAR* Ind()
	{
		return *Get().CurrentIndent();
	}
};

class FScopeLogIndent
{
public:
	UE_NODISCARD_CTOR FScopeLogIndent()
	{
		FDebugLogger::Get().Indent();
	}
	~FScopeLogIndent()
	{
		FDebugLogger::Get().UnIndent();
	}
};